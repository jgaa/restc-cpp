#include <assert.h>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/DataReaderStream.h"
#include "restc-cpp/error.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {


class ChunkedReaderImpl : public DataReader {
public:

    ChunkedReaderImpl(add_header_fn_t&& fn, unique_ptr<DataReaderStream>&& source)
    : stream_{move(source)}, add_header_(move(fn))
    {
    }

    bool IsEof() const override {
        return stream_->IsEof();
    }

    void Log(const boost::asio::const_buffers_1 buffers, const char *tag) {
        size_t seg = 0;

        // At the time of the implementation, there are never multiple buffers.
        // However, if this change in the future, this logging will still be correct.
        // It may save some time if other propbels occur.
        for(const auto buffer : buffers) {

            boost::string_ref sr {
                boost::asio::buffer_cast<const char *>(buffer),
                boost::asio::buffer_size(buffer)
            };

            RESTC_CPP_LOG_TRACE << tag << ' ' << seg << "# "
                << boost::asio::buffer_size(buffer) << " bytes: "  << sr;

            ++seg;
        }
    }

    boost::asio::const_buffers_1 ReadSome() override {

        EatPadding();

        if (stream_->IsEof()) {
            return {nullptr, 0};
        }

        if (chunk_len_ == 0) {
            RESTC_CPP_LOG_TRACE << "ChunkedReaderImpl::ReadSome(): Need new chunk.";
            chunk_len_ = GetNextChunkLen();
            if (chunk_len_ == 0) {
                // Read the trailer
                RESTC_CPP_LOG_TRACE << "ChunkedReaderImpl::ReadSome(): End of chunked stream - reading headers";
                stream_->ReadHeaderLines(add_header_);
                stream_->SetEof();
                RESTC_CPP_LOG_TRACE << "ChunkedReaderImpl::ReadSome(): End of chunked stream. Done.";
                return {nullptr, 0};
            }
        }

        auto data = GetData();

        Log(data, "ChunkedReaderImpl::ReadSome()");

        return data;
    }

private:
    void EatPadding() {
        if (eat_chunk_padding_) {
            eat_chunk_padding_ = false;

            char ch = {};
            if ((ch = stream_->Getc()) != '\r') {
                throw ParseException("Chunk: Missing padding CR!");
            }

            if ((ch = stream_->Getc()) != '\n') {
                throw ParseException("Chunk: Missing padding LF!");
            }
        }
    }

    boost::asio::const_buffers_1 GetData() {

        auto rval = stream_->GetData(chunk_len_);
        const auto seg_len = boost::asio::buffer_size(rval);
        chunk_len_ -= seg_len;

        if (chunk_len_ == 0) {
                eat_chunk_padding_ = true;
        }

        return rval;
    }

    size_t GetNextChunkLen() {
        size_t chunk_len = 0;
        char ch = stream_->Getc();

        if (!isxdigit(ch)) {
            throw ParseException("Missing chunk-length in new chunk.");
        }

        for(; isxdigit(ch); ch = stream_->Getc()) {
            chunk_len *= 16;
            if (ch >= 'a') {
                chunk_len += 10 + (ch - 'a');
            } else if (ch >= 'A') {
                chunk_len += 10 + (ch - 'A');
            } else {
                chunk_len += ch - '0';
            }
        }

        for(; ch != '\r'; ch = stream_->Getc())
            ;

        if (ch != '\r') {
            throw ParseException("Missing CR in first chunk line");
        }

        if ((ch = stream_->Getc()) != '\n') {
            throw ParseException("Missing LF in first chunk line");
        }

        RESTC_CPP_LOG_TRACE << "ChunkedReaderImpl::GetNextChunkLen(): " << chunk_len;

        return chunk_len;
    }

    size_t chunk_len_ = 0;
    bool eat_chunk_padding_ = false;
    std::unique_ptr<DataReaderStream> stream_;
    //boost::string_ref buffer;
    add_header_fn_t add_header_;
};

DataReader::ptr_t
DataReader::CreateChunkedReader(add_header_fn_t fn, unique_ptr<DataReaderStream>&& source) {
    return make_unique<ChunkedReaderImpl>(move(fn), move(source));
}


} // namespace

