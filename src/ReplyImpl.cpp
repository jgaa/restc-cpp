
#include "restc-cpp/logging.h"
#include "restc-cpp/helper.h"
#include "ReplyImpl.h"

namespace restc_cpp {

boost::optional< string > ReplyImpl::GetHeader(const string& name) {
    boost::optional< string > rval;

    auto it = headers_.find(name);
    if (it != headers_.end()) {
        rval = it->second;
    }

    return rval;
}

ReplyImpl::~ReplyImpl() {
    if (connection_ && connection_->GetSocket().IsOpen()) {
        try {
            RESTC_CPP_LOG_TRACE << "~ReplyImpl(): " << *connection_
                << " is still open. Closing it to prevent problems with partially "
                << "received data.";
            connection_->GetSocket().Close();
            connection_.reset();
        } catch(std::exception& ex) {
            RESTC_CPP_LOG_WARN << "~ReplyImpl(): Caught exception:" << ex.what();
        }
    }
}


void ReplyImpl::StartReceiveFromServer() {
    static const std::string content_len_name{"Content-Length"};
    static const std::string connection_name{"Connection"};
    static const std::string keep_alive_name{"keep-alive"};
    static const std::string transfer_encoding_name{"Transfer-Encoding"};
    static const std::string chunked_name{"chunked"};

    read_buffer_ = make_unique<buffer_t>();

    // Get the header part of the message into header_
    ReadHeaderAndMayBeSomeMore();
    ParseHeaders();

    RESTC_CPP_LOG_TRACE << "### Initial read: header_.size() + crlf = "
        << (header_.size() + 4)
        << ", body_.size() = " << body_.size()
        << ", totals to " << (body_.size() + header_.size() + 4)
        ;

    assert((body_.size() + header_.size() + 4 /* crlf */ ) == data_bytes_received_);

    // TODO: Handle redirects

    if (const auto cl = GetHeader(content_len_name)) {
        content_length_ = stoi(*cl);

        if (*content_length_ < body_.size()) {
            RESTC_CPP_LOG_DEBUG << "*** Invalid content_length_ = " << *content_length_
                << ", body_.size() = " << body_.size()
                << ". Tagging " << *connection_ << " for close.";

            do_close_connection_ = true;
        }

        body_bytes_received_ = body_.size();

    } else {
        auto te = GetHeader(transfer_encoding_name);
        if (te && boost::iequals(*te, chunked_name)) {
            PrepareChunkedPayload();
        }
    }

    // TODO: Check for Connection: close header and tag the connection for close
    const auto conn_hdr = GetHeader(connection_name);
    if (!conn_hdr || !ciEqLibC()(*conn_hdr, keep_alive_name)) {
        string value = *conn_hdr;
        RESTC_CPP_LOG_TRACE << "No 'Connection: keep-alive' header. "
            << "Tagging " << *connection_ << " for close.";
        do_close_connection_ = true;
    }

    CheckIfWeAreDone();
}

boost::asio::const_buffers_1
ReplyImpl::GetSomeData()  {
    // We have some data ready to serve.
    if (!body_.empty()) {

        auto rval = boost::asio::const_buffers_1{body_.data(), body_.size()};
        body_.clear();
        return rval;
    }

    if (IsChunked())
        return DoGetSomeChunkedData();
    return DoGetSomeData();
}


string ReplyImpl::GetBodyAsString() {
    std::string buffer;

    while(MoreDataToRead()) {
        auto data = GetSomeData();
        buffer.append(boost::asio::buffer_cast<const char*>(data),
                        boost::asio::buffer_size(data));
    }

    return buffer;
}

void ReplyImpl::CheckIfWeAreDone() {
    if (!have_received_all_data_) {
        if (IsChunked()) {
            if (chunked_ == ChunkedState::DONE) {
                have_received_all_data_ = true;
                RESTC_CPP_LOG_TRACE << "Have received all data in the current request.";
            }
        } else {
            if ((content_length_
                && (*content_length_ <= body_bytes_received_))
            || (this->GetResponseCode() == 204)) {

                have_received_all_data_ = true;
                RESTC_CPP_LOG_TRACE  << "Have received all data in the current request.";
            }
        }

        if (have_received_all_data_) {
            ReleaseConnection();
        }
    }
}

void ReplyImpl::ReleaseConnection() {
    if (do_close_connection_) {
        RESTC_CPP_LOG_TRACE << "Closing connection because do_close_connection_ is true: "
            << *connection_;
        if (connection_->GetSocket().IsOpen()) {
            connection_->GetSocket().Close();
        }
    }

    connection_.reset();
}

void ReplyImpl::ParseHeaders(bool skip_requestline) {
    static const string crlf{"\r\n"};
    static const string expected_protocol{"HTTP/1.1"};

    //owner_.LogDebug(header_.to_string());

    auto remaining = header_;

    if (!skip_requestline) {
        auto pos = remaining.find(crlf);
        if (pos == remaining.npos) {
            throw runtime_error(
                "Invalid header - malformed response line - no CRLF");
        }
        status_line_ = {remaining.data(), pos};

        pos = status_line_.find(' ');
        if (pos == status_line_.npos) {
            throw runtime_error(
                "Invalid header - malformed response line");
        }

        const auto protocol = boost::string_ref(status_line_.data(), pos);

        if (strncasecmp(protocol.data(),
            expected_protocol.c_str(),
            expected_protocol.size())) {

            throw runtime_error("Invalid header - Unexpected protocol");
        }

        if (status_line_.size() < (pos + 3)) {
            throw runtime_error(
                "Invalid header - malformed response line - status code");
        }

        auto code = boost::string_ref(status_line_.data() + pos + 1, 3);

        if (status_line_.size() > (pos + 5)) {
            status_code_ = stoi(code.to_string());
            status_message_ = {
                status_line_.data() + pos + 5,
                status_line_.size() - pos - 5};
        }
    }

    bool done = remaining.empty();
    while(!done) {
        // Get Next Line
        auto start_of_line = remaining.find(crlf);
        if (start_of_line == remaining.npos) {
            throw runtime_error("Invalid header - missing CRLF");
        }
        start_of_line += 2;
        remaining = {remaining.data() + start_of_line,
            remaining.size() - start_of_line};
        auto end_of_line = remaining.find(crlf);
        if (end_of_line == remaining.npos) {
            done = true;
            if (remaining.empty()) {
                return;
            }
            end_of_line = remaining.size();
        }

        auto line = boost::string_ref(remaining.data(), end_of_line);

        // Get Name
        boost::string_ref name;
        std::string value;
        enum class State { PARSE_NAME, PARSE_DELIM, PARSE_VALUE };
        State state = State::PARSE_NAME;
        for(auto ch = line.cbegin() ; ch != line.cend(); ++ch) {
            if (!value.empty())
                break; // Done

            if (*ch == ' ' || *ch == ':') {
                switch(state) {
                    case State::PARSE_NAME:
                        name = {line.data(),
                            static_cast<size_t>(ch - line.begin())};
                        state = State::PARSE_DELIM;
                        break;
                    case State::PARSE_DELIM:
                        break; // Skip
                    case State::PARSE_VALUE:
                        break; // Consume
                }
            } else {
                switch(state) {
                    case State::PARSE_NAME:
                        break; // Consume
                    case State::PARSE_DELIM:
                        {
                            state = State::PARSE_VALUE;
                            const ptrdiff_t cont_len = line.cend() - ch;
                            assert(cont_len > 0);
                            assert(cont_len <= (ptrdiff_t)line.size());
                            value.assign(ch, static_cast<size_t>(cont_len));
                        }
                        break;
                    case State::PARSE_VALUE:
                        break;
                } // switch
            } // else *ch == ' ' || *ch == ':'
        } // for(auto ch : line)

        // Is the line wrapped? If so, append the value.
        while ((remaining.size() > 2) && (remaining.at(2) == ' ')) {
            assert(remaining.at(0) == '\r');
            assert(remaining.at(1) == '\n');

            // Wrapped line.
            remaining = {remaining.data() + 2, remaining.size() - 2};
            end_of_line = remaining.find(crlf);
            if (end_of_line == remaining.npos) {
                throw runtime_error("Invalid header - missing CRLF");
            }

            value.append(remaining.data() + 1, end_of_line - 1);

            remaining = {remaining.data() + end_of_line,
                remaining.size() - end_of_line};
        } // while remaining.size() > 2

        if (!name.empty()) {
            headers_[name.to_string()] = value;
        }
    }
}

size_t ReplyImpl::ReadHeaderAndMayBeSomeMore(size_t bytes_used) {
    static string end_of_header{"\r\n\r\n"};

    auto timer = IoTimer::Create(
            owner_.GetConnectionProperties()->replyTimeoutMs,
            owner_.GetIoService(), connection_);

    while(true) {
        if (BYTES_AVAILABLE < 1) {
            throw runtime_error("Header is too long - out of buffer space");
        }

        auto received_buffer = ReadSomeData(read_buffer_->data() + bytes_used,
                                        BYTES_AVAILABLE, false /* no timer */);

        const size_t received = boost::asio::buffer_size(received_buffer);

        if (received == 0) {
            throw runtime_error("Received 0 bytes on read.");
        }

        bytes_used += received;

        buffer_ = {read_buffer_->data(), bytes_used};
        auto pos = buffer_.find(end_of_header);
        if (pos != buffer_.npos) {
            header_ = buffer_ = {read_buffer_->data(), pos};
            body_ = {header_.end() + 4, bytes_used - 4 - header_.size()};
            return bytes_used;
        }
    }
}

void ReplyImpl::PrepareChunkedPayload() {
    assert(chunked_ == ChunkedState::NOT_CHUNKED);
    buffer_ = body_;
    body_.clear();
    chunked_ = ChunkedState::GET_SIZE;
}

bool ReplyImpl::ProcessChunkHeader(boost::string_ref buffer) {
        static const string crlf{"\r\n"};

        chunked_ = ChunkedState::GET_SIZE;
        current_chunk_len_ = 0;
        current_chunk_read_ = 0;
        body_.clear();

        if (buffer.size() < static_cast<decltype(buffer.size())>(current_chunk_ ? 6 : 3)) {// smallest possible header length
            return false;
        }

        size_t seg_len = 0;

        if (current_chunk_) {
            if ((buffer[0] != '\r') || (buffer[1] != '\n')) {
                throw runtime_error("Missing CRLF before HTTP 1.1 chunk header!");
            }

            // Shrink buffer so that it starts with the segment length
            buffer = {buffer.begin() + 2, buffer.size() - 2};
        }

        for(auto it = buffer.cbegin(); it != buffer.cend(); ++it) {
            if (std::isxdigit(*it)) {
                if (++seg_len > 7) {
                    throw runtime_error("More than 7 hex digit in HTTP 1.1 chunk length!");
                }
                continue;
            }
            break;
        }


        if (!seg_len) {
            throw runtime_error("Chunked segment must start with a hex digit.");
        }


        auto pos = buffer.find(crlf);
        if (pos == buffer.npos) {
            return false;
        }
        pos += 2; // crlf

        const auto seg = boost::string_ref(buffer.data(), seg_len);
        current_chunk_len_ = std::stoul(seg.to_string(), nullptr, 16);

        RESTC_CPP_LOG_TRACE << "---> Chunked header: Segment lenght is "
            << current_chunk_len_ << " bytes." ;

        if (current_chunk_len_ == 0) {
            // Last segment. No more payload.

            body_ = {buffer.data() + pos, buffer.size() - pos};

            if (body_.compare(crlf) == 0) {
                // We have a complete last segment. let's end this.
                body_.clear();
                chunked_ = ChunkedState::DONE;
                RESTC_CPP_LOG_TRACE << "chunked_ = ChunkedState::DONE [simple]" ;
            } else {
                chunked_ = ChunkedState::IN_TRAILER;
                // Clean up the buffer so we are prepared to receive headers
                size_t offset = 2;
                memcpy(read_buffer_->data(), "\r\n", 2); // The header parser may need this
                if (!body_.empty()) {
                    assert(body_.size() + 2 <= read_buffer_->size());
                    memmove(read_buffer_->data() + offset, body_.data(), body_.size());
                    offset += body_.size();
                    body_.clear();
                    buffer_.clear();
                    header_.clear();
                }

                // Finish up. Read trailer, process headers.
                ReadHeaderAndMayBeSomeMore(offset);
                ParseHeaders(true /* No request line */ );
                chunked_ = ChunkedState::DONE;
                RESTC_CPP_LOG_TRACE << "chunked_ = ChunkedState::DONE [fetched trailer]" ;

                if (body_.size()) {
                    assert(false && "Received data after last chunk");
                    // TODO: Tag for close - the connection cannot be reused
                }
            }
        } else {
            assert(current_chunk_len_ > 0);

            /* Adjust the body to the actual payload.
             * If we crop the body (the socket may received
             * more data than the the current chunk, the remaining
             * part will still be visible in buffer.
             */
            body_ = {buffer.data() + pos,
                std::min(buffer.size() - pos, current_chunk_len_)};

            chunked_ = ChunkedState::IN_SEGMENT;
        }

        ++current_chunk_;
        return true;
    }

    boost::asio::const_buffers_1
    ReplyImpl::DoGetSomeChunkedData() {
    assert(body_.empty());

    if (chunked_ == ChunkedState::GET_SIZE) {

        auto work_buffer = buffer_;
        size_t offset = 0;
        bool virgin = true;

        while(!ProcessChunkHeader(work_buffer)) {

            /* Move whatever data was left in buffer_ to the start
                * of read_buffer, so that ProcessChunkHeader can use
                * it together with received data
                */
            if (virgin) {
                virgin = false;
                if (!buffer_.empty()) {
                    offset = buffer_.size();
                    memmove(read_buffer_->data(),
                            buffer_.begin(),
                            buffer_.size());
                    buffer_.clear();
                }
            }

            if (offset >= read_buffer_->size()) {
                throw runtime_error("Out of buffer-space reading chunk header");
            }

            const auto rcvd_buffer = ReadSomeData(
                (read_buffer_->data() + offset),
                read_buffer_->size() - offset);

            const auto bytes_received = boost::asio::buffer_size(rcvd_buffer);

            offset += bytes_received;
            buffer_ = {read_buffer_->data(), offset};
            work_buffer = buffer_;
        }

        if (body_.empty()) {
            buffer_.clear(); // No remaining data
        } else {
            boost::asio::const_buffers_1 rval{
                body_.begin(), body_.size()};

            // Shrink the buffer to whatever is left after body.
            buffer_ = {body_.end(),
                static_cast<size_t>(buffer_.end() - body_.end())};

            if (chunked_ == ChunkedState::IN_SEGMENT) {
                current_chunk_read_ = body_.size();
                body_bytes_received_ += body_.size();
                body_.clear();

                RESTC_CPP_LOG_TRACE << "### Got chunked header: current_chunk_read_ = "
                    << current_chunk_read_
                    << ", current_chunk_len_ = " << current_chunk_len_
                    << ", remaining = " << ((int)current_chunk_len_ - (int)current_chunk_read_)
                    << ", buffer size = " << boost::asio::buffer_size(rval)
                    ;

                return rval;
            }
        }
    }

    if (chunked_ == ChunkedState::IN_SEGMENT) {
        auto want_bytes = current_chunk_len_ - current_chunk_read_;

        if (want_bytes == 0) {
            chunked_ = ChunkedState::GET_SIZE;
            return DoGetSomeChunkedData(); // Need to
        }

        if (!buffer_.empty()) {
                // Return data from the existing buffer
            return TakeSegmentDataFromBuffer();
        }

        assert(buffer_.empty());
        const auto rcvd_buffer = ReadSomeData(
                (read_buffer_->data()), read_buffer_->size());

        const auto bytes_received = boost::asio::buffer_size(rcvd_buffer);

        buffer_ = {read_buffer_->data(), bytes_received};

        return TakeSegmentDataFromBuffer();
    }

    if (chunked_ == ChunkedState::IN_TRAILER) {
        assert(false);
    }

    CheckIfWeAreDone();

    return {nullptr, 0};
}

boost::asio::const_buffers_1
ReplyImpl::TakeSegmentDataFromBuffer() {
    auto want_bytes = current_chunk_len_ - current_chunk_read_;
    assert(want_bytes);

    const boost::string_ref rval = {buffer_.data(),
        std::min(buffer_.size(), want_bytes)};

    current_chunk_read_ += rval.size();
    body_bytes_received_ += rval.size();

    // Shrink the buffer to whatever is left after body.
    buffer_ = {rval.end(), buffer_.size() - rval.size()};

    RESTC_CPP_LOG_TRACE << "### TakeSegmentDataFromBuffer: current_chunk_read_ = "
        << current_chunk_read_
        << ", current_chunk_len_ = " << current_chunk_len_
        << ", remaining = " << ((int)current_chunk_len_ - (int)current_chunk_read_)
        << ", rval.size = " <<  rval.size()
        ;

    return {rval.data(), rval.size()};
}


boost::asio::const_buffers_1 ReplyImpl::DoGetSomeData() {
    if (have_received_all_data_)
        return  {nullptr, 0};

    // Determine how much data we should read
    size_t want_bytes = 0;

    if (content_length_) {
        const auto bytes_left = *content_length_ - body_bytes_received_;
        want_bytes = std::min(bytes_left, read_buffer_->size());
    } else {
        assert("No content-length");
        throw runtime_error("No content-length");
    }

    const auto return_buffer = ReadSomeData(read_buffer_->data(),
                                            want_bytes);

    body_bytes_received_ += boost::asio::buffer_size(return_buffer);
    CheckIfWeAreDone();

    return return_buffer;
}

boost::asio::const_buffers_1
ReplyImpl::ReadSomeData(char *ptr, size_t bytes, bool with_timer) {
    assert(bytes);
    size_t received = 0;

    IoTimer::ptr_t timer;
    if (with_timer) {
        auto timer = IoTimer::Create(
            owner_.GetConnectionProperties()->replyTimeoutMs,
            owner_.GetIoService(), connection_);
    }

    received = AsyncReadSome({ptr, bytes});

    if (timer) {
        timer->Cancel();
    }

    if (received == 0) {
        throw runtime_error("Got 0 bytes from server");
    }

    RESTC_CPP_LOG_TRACE << "---> Received " << received << " bytes: "
        << "\r\n--------------- RECEIVED START --------------\r\n"
        << boost::string_ref(ptr, received)
        << "\r\n--------------- RECEIVED END --------------";

    data_bytes_received_ += received;

    return {ptr, received};
}

size_t ReplyImpl::AsyncReadSome(boost::asio::mutable_buffers_1 read_buffers) {

    assert(connection_);
    if (!connection_) {
        throw runtime_error("AsyncReadSome(): Connection is released");
    }
    return connection_->GetSocket().AsyncReadSome(read_buffers,
                                                    ctx_.GetYield());
}

std::unique_ptr<Reply>
Reply::Create(Connection::ptr_t connection,
       Context& ctx,
       RestClient& owner) {

    return make_unique<ReplyImpl>(move(connection), ctx, owner);
}

} // restc_cpp
