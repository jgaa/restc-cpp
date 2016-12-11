
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/error.h"

using namespace std;

namespace restc_cpp {


class ChunkedReaderImpl : public DataReader {
public:

    ChunkedReaderImpl(add_header_fn_t&& fn, ptr_t&& source)
    :  stream_{move(source)}, add_header_(move(fn))
    {

    }

    bool IsEof() const override {
        return stream_.IsEof();
    }

    boost::asio::const_buffers_1 ReadSome() override {

        if (stream_.IsEof()) {
            return {nullptr, 0};
        }

        if (chunk_len_ == 0) {
            chunk_len_ = GetNextChunkLen();
            if (chunk_len_ == 0) {
                // Read the trailer
                stream_.ReadHeaderLines(add_header_);
                stream_.SetEof();
                return {nullptr, 0};
            }
        }

        return GetData();
    }

private:
    boost::asio::const_buffers_1 GetData() {

        auto rval = stream_.GetData(chunk_len_);
        const auto seg_len = boost::asio::buffer_size(rval);
        chunk_len_ -= seg_len;

        if (chunk_len_ == 0) {
            // Eat padding CRLF
            char ch;
            if ((ch = stream_.Getc()) != '\r') {
                throw ParseException("Chunk: Missing padding CR!");
            }

            if ((ch = stream_.Getc()) != '\n') {
                throw ParseException("Chunk: Missing padding LF!");
            }
        }

        return rval;
    }

    size_t GetNextChunkLen() {
        size_t chunk_len = 0;
        char ch =  stream_.Getc();

        if (!isxdigit(ch)) {
            throw ParseException("Missing chunk-length in new chunk.");
        }

        for(; isxdigit(ch); ch= stream_.Getc()) {
            chunk_len *= 16;
            if (ch >= 'a') {
                chunk_len += 10 + (ch - 'a');
            } else if (ch >= 'A') {
                chunk_len += 10 + (ch - 'A');
            } else {
                chunk_len += ch - '0';
            }
        }

        for(; ch != '\r'; ch = stream_.Getc())
            ;

        if (ch != '\r') {
            throw ParseException("Missing CR in first chunk line");
        }

        if ((ch = stream_.Getc()) != '\n') {
            throw ParseException("Missing LF in first chunk line");
        }

        return chunk_len;
    }

    size_t chunk_len_ = 0;
    DataReaderStream stream_;
    boost::string_ref buffer;
    add_header_fn_t add_header_;
};

DataReader::ptr_t
DataReader::CreateChunkedReader(add_header_fn_t fn, ptr_t&& source) {
    return make_unique<ChunkedReaderImpl>(move(fn), move(source));
}


} // namespace

