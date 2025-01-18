#include <cassert>
#include <clocale>
#include <ios>

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
    : stream_{std::move(source)}, add_header_(std::move(fn))
    {
    }

    [[nodiscard]] bool IsEof() const override { return stream_->IsEof(); }

    void Finish() override {
        ReadSome();
        if (!IsEof()) {
            throw ProtocolException("Failed to finish chunked payload");
        }

        if (stream_) {
            stream_->Finish();
        }
    }

    [[nodiscard]] static string ToPrintable(boost::string_ref buf)
    {
        ostringstream out;
        locale const loc;
        auto pos = 0;
        out << '\n';

        for(const auto ch : buf) {
            if ((++pos % line_length) == 0u) {
                out << '\n';
            }
            if (std::isprint(ch, loc)) {
                out << ch;
            } else {
                out << '.';
            }
        }

        return out.str();
    }

    static void Log(const ::restc_cpp::boost_const_buffer buffers, const char * /*tag*/)
    {
        const auto buf_len = boost_buffer_size(buffers);

        // At the time of the implementation, there are never multiple buffers.
        RESTC_CPP_LOG_TRACE_(tag << ' ' << "# " << buf_len
            << " bytes: "
            << ToPrintable({
                boost::asio::buffer_cast<const char *>(*buffers.begin()),
                           buf_len}));
    }

    ::restc_cpp::boost_const_buffer ReadSome() override {

        EatPadding();

        if (stream_->IsEof()) {
            return {nullptr, 0};
        }

        if (chunk_len_ == 0) {
            RESTC_CPP_LOG_TRACE_("ChunkedReaderImpl::ReadSome(): Need new chunk.");
            chunk_len_ = GetNextChunkLen();
            RESTC_CPP_LOG_TRACE_("ChunkedReaderImpl::ReadSome(): "
                << "Next chunk is " << chunk_len_ << " bytes ("
                << hex << chunk_len_ << " hex)");
            if (chunk_len_ == 0) {
                // Read the trailer
                RESTC_CPP_LOG_TRACE_("ChunkedReaderImpl::ReadSome(): End of chunked stream - reading headers");
                stream_->ReadHeaderLines(add_header_);
                stream_->SetEof();
                RESTC_CPP_LOG_TRACE_("ChunkedReaderImpl::ReadSome(): End of chunked stream. Done.");
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

            if (stream_->Getc() != '\r') {
                throw ParseException("Chunk: Missing padding CR!");
            }

            if (stream_->Getc() != '\n') {
                throw ParseException("Chunk: Missing padding LF!");
            }
        }
    }

    ::restc_cpp::boost_const_buffer GetData() {

        auto rval = stream_->GetData(chunk_len_);
        const auto seg_len = boost::asio::buffer_size(rval);
        chunk_len_ -= seg_len;

        if (chunk_len_ == 0) {
                eat_chunk_padding_ = true;
        }

        return rval;
    }

    size_t GetNextChunkLen() {
        static constexpr size_t magic_16 = 16;
        static constexpr size_t magic_10 = 10;
        size_t chunk_len = 0;
        char ch = stream_->Getc();

        if (isxdigit(ch) == 0) {
            throw ParseException("Missing chunk-length in new chunk.");
        }

        for (; isxdigit(ch) != 0; ch = stream_->Getc()) {
            chunk_len *= magic_16;
            if (ch >= 'a') {
                chunk_len += magic_10 + (ch - 'a');
            } else if (ch >= 'A') {
                chunk_len += magic_10 + (ch - 'A');
            } else {
                chunk_len += ch - '0';
            }
        }

        for (; ch != '\r'; ch = stream_->Getc()) {
            ;
        }

        if (ch != '\r') {
            throw ParseException("Missing CR in first chunk line");
        }

        if ((ch = stream_->Getc()) != '\n') {
            throw ParseException("Missing LF in first chunk line");
        }

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
    return make_unique<ChunkedReaderImpl>(std::move(fn), std::move(source));
}


} // namespace

