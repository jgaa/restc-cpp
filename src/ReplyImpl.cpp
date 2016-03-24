#include <iostream>
#include <thread>
#include <future>
#include <unordered_map>

#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>

#include "restc-cpp/restc-cpp.h"

using namespace std;

namespace restc_cpp {

#define BYTES_AVAILABLE (read_buffer_->size() - bytes_used)

class ReplyImpl : public Reply {
public:
    using buffer_t = std::array<char, 1024 * 16>;

    ReplyImpl(Connection::ptr_t& connection,
              Context& ctx,
              RestClient& owner)
    : connection_{connection}, ctx_{ctx}, owner_{owner}
    {
    }

    boost::optional< string > GetHeader(const string& name) override {
        boost::optional< string > rval;

        auto it = headers_.find(name);
        if (it != headers_.end()) {
            rval = it->second;
        }

        return rval;
    }

    void StartReceiveFromServer() override {
        static const std::string content_len_name{"Content-Length"};

        read_buffer_ = make_unique<buffer_t>();

        // Get the header part of the message into header_
        data_bytes_received_ = ReadHeaderAndMayBeSomeMore();

        ParseHeaders();

        // Check for errors
        if (GetResponseCode() >= 400) {
            throw runtime_error("The request failed");
        }

        // TODO: Handle redirects

        auto cl = GetHeader(content_len_name);
        if (cl) {
            content_length_ = stoi(*cl);
        }

        // TODO: Handle chunked responses

        body_bytes_received_ = body_.size();
        CheckIfWeAreDone();
    }

    int GetResponseCode() const override { return status_code_; }

    boost::asio::const_buffers_1 GetSomeData() override {
        if (!body_.empty()) {

            auto rval = boost::asio::const_buffers_1{body_.data(), body_.size()};
            body_.clear();
            return rval;
        }

        if (have_received_all_data_)
            return  boost::asio::const_buffers_1{nullptr, 0};

        // Determine how much data we should read
        size_t want_bytes = 0;

        if (content_length_) {
            const auto bytes_left = *content_length_ - body_bytes_received_;
            want_bytes = std::min(bytes_left, read_buffer_->size());
        } else {
            // TODO: Implement chunked mode
            assert("Chunked response not yet implemented");
        }

        assert(want_bytes);

        const size_t received = connection_->GetSocket().AsyncReadSome(
            {read_buffer_->data(), want_bytes}, ctx_.GetYield());

        if (received == 0) {
            throw runtime_error("Got 0 bytes from server");
        }

        data_bytes_received_ += received;
        body_bytes_received_ += received;

        CheckIfWeAreDone();

        return {read_buffer_->data(), received};
    }

    string GetBodyAsString() override {
        std::string buffer;

        while(MoreDataToRead()) {
            auto data = GetSomeData();
            buffer.append(boost::asio::buffer_cast<const char*>(data),
                          boost::asio::buffer_size(data));
        }

        return buffer;
    }

    bool MoreDataToRead() override {
        return !body_.empty() || !have_received_all_data_;
    }

private:
    void CheckIfWeAreDone() {
        if (!have_received_all_data_
            && content_length_
            && (*content_length_ == body_bytes_received_)) {

            have_received_all_data_ = true;
            owner_.LogDebug("Have received all data in the current request.");
        }
    }


    void ParseHeaders() {
        static const string crlf{"\r\n"};
        static const string expected_protocol{"HTTP/1.1"};

        owner_.LogDebug(header_.to_string());

        auto remaining = header_;

        {
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

        while(true) {
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
                // We are done.
                return;
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
                            state = State::PARSE_VALUE;
                            value.assign(ch,
                                         static_cast<size_t>(line.cend() - ch));
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


    size_t ReadHeaderAndMayBeSomeMore() {
        size_t bytes_used = 0;
        static string end_of_header{"\r\n\r\n"};

        while(true) {
            if (BYTES_AVAILABLE < 1) {
                throw runtime_error("Header is too long - out of buffer space");
            }

            const size_t received = connection_->GetSocket().AsyncReadSome(
                {read_buffer_->data() + bytes_used, BYTES_AVAILABLE}, ctx_.GetYield());

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

    Connection::ptr_t connection_;
    Context& ctx_;
    RestClient& owner_;
    std::vector<char> memory_buffer_;
    boost::string_ref buffer_;
    boost::string_ref status_line_;;
    boost::string_ref header_;
    boost::string_ref body_;
    int status_code_ = 0;
    boost::string_ref status_message_;
    map<string, string, ciLessLibC> headers_;
    bool have_received_all_data_ = false;
    std::unique_ptr<buffer_t> read_buffer_;
    boost::optional<size_t> content_length_;
    size_t data_bytes_received_ = 0;
    size_t body_bytes_received_ = 0;
};


std::unique_ptr<Reply>
Reply::Create(Connection::ptr_t& connection,
       Context& ctx,
       RestClient& owner) {

    return make_unique<ReplyImpl>(connection, ctx, owner);
}

} // restc_cpp

