
#include "restc-cpp/DataReader.h"
#include "restc-cpp/error.h"

using namespace std;

namespace restc_cpp {

void DataReaderStream::Fetch() {
    if (++curr_ >= end_) {
        auto buf = source_->ReadSome();
        curr_ = boost::asio::buffer_cast<const char *>(buf);
        end_ = curr_ + boost::asio::buffer_size(buf);
        eof_ = curr_ == end_;
        if (eof_) {
            throw ProtocolException("Fetch(): EOF");
        }
    }
}

boost::asio::const_buffers_1
DataReaderStream::ReadSome() {
    Fetch();
    boost::asio::const_buffers_1 rval = {curr_,
        static_cast<size_t>(end_ - curr_)};
    curr_ = end_;
    return rval;
}

boost::asio::const_buffers_1
DataReaderStream::GetData(size_t maxBytes) {
    Fetch();

    const auto diff = end_ - curr_;
    assert(diff >= 0);
    const auto seg_len = std::min<size_t>(maxBytes, diff);
    boost::asio::const_buffers_1 rval = {curr_, seg_len};
    if (seg_len > 0) {
        curr_ += seg_len - 1;
    }
    return rval;
}


void DataReaderStream::ReadServerResponse(Reply::HttpResponse& response)
{
    string http_1_1{"HTTP/1.1"};
    char ch = {};

    // Get HTTP version
    std::string value;
    for(ch = Getc(); ch != ' '; ch = Getc()) {
        value += ch;
        if (value.size() > 16) {
            throw ProtocolException("ReadHeaders(): Too much HTTP version!");
        }
    }
    if (ch != ' ') {
        throw ProtocolException("ReadHeaders(): No space after HTTP version");
    }
    if (value.empty()) {
        throw ProtocolException("ReadHeaders(): No HTTP version");
    }
    if (ciEqLibC()(value, http_1_1)) {
        ; // Do nothing HTTP 1.1 is the default value
    } else {
        throw ProtocolException(
            string("ReadHeaders(): unsupported HTTP version: ") + value);
    }

    // Get response code
    value.clear();
    for(ch = Getc(); ch != ' '; ch = Getc()) {
        value += ch;
        if (value.size() > 3) {
            throw ProtocolException("ReadHeaders(): Too much HTTP response code!");
        }
    }
    if (value.size() != 3) {
        throw ProtocolException(
            string("ReadHeaders(): Incorrect length of HTTP response code!: ")
            + value);
    }

    response.status_code = stoi(value);

    if (ch != ' ') {
        throw ProtocolException("ReadHeaders(): No space after HTTP response code");
    }

    // Get response text
    value.clear();
    for(ch = Getc(); ch != '\r'; ch = Getc()) {
        value += ch;
        if (value.size() > 256) {
            throw ConstraintException("ReadHeaders(): Too long HTTP response phrase!");
        }
    }

    // Skip CRLF
    assert(ch == '\r');
    ch = Getc();
    if (ch != '\n') {
        throw ProtocolException("ReadHeaders(): No CR/LF after HTTP response phrase!");
    }

    response.reason_phrase = move(value);
}

void DataReaderStream::ReadHeaderLines(const add_header_fn_t& addHeader) {
    while(true) {
        char ch;
        std::string name, value;
        for(ch = Getc(); ch != '\r'; ch = Getc()) {
            if (ch == ' ' || ch == '\t') {
                continue;
            }
            if (ch == ':') {
                value = GetHeaderValue();
                ch = '\n';
                break;
            }
            name += ch;
            if (name.size() > 256) {
                throw ConstraintException("Chunk Trailer: Header name too long!");
            }
        }

        if (ch == '\r') {
            ch = Getc();
        }

        if (ch != '\n') {
            throw ProtocolException("Chunk Trailer: Missing LF after parse!");
        }

        if (name.empty()) {
            if (!value.empty()) {
                throw ProtocolException("Chunk Trailer: Header value without name!");
            }
            return; // An empty line marks the end of the trailer
        }

        if (++num_headers_ > 256) {
            throw ConstraintException("Chunk Trailer: Too many lines in header!");
        }
        addHeader(move(name), move(value));
        name.clear();
        value.clear();
    }
}

std::string DataReaderStream::GetHeaderValue() {
    std::string value;
    char ch;

    while(true) {
        for (ch = Getc(); ch == ' ' || ch == '\t'; ch = Getc())
            ; // skip space

        for (; ch != '\r'; ch = Getc()) {
            value += ch;
            if (value.size() > (1024 * 4)) {
                throw ConstraintException("Chunk Trailer: Header value too long!");
            }
        }

        if (ch != '\r') {
            throw ProtocolException("Chunk Trailer: Missing CR!");
        }

        if ((ch = Getc()) != '\n') {
            throw ProtocolException("Chunk Trailer: Missing LF!");
        }

        // Peek
        ch = Getc();
        if ((ch != ' ') && (ch != '\t')) {
            Ungetc();
            return value;
        }

        value += ' ';
    }
}


} // namespace
