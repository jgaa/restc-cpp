#include <zlib.h>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {

class ZipReaderImpl : public DataReader {
public:
    enum class Format { DEFLATE, GZIP };

    ZipReaderImpl(std::unique_ptr<DataReader>&& source,
                const Format format)
    : source_{std::move(source)}
    {
        const auto wsize = (format == Format::GZIP) ? (MAX_WBITS | 16) : MAX_WBITS;

        if (inflateInit2(&strm_, wsize) != Z_OK) {
            throw DecompressException("Failed to initialize decompression");
        }
    }

    ZipReaderImpl(const ZipReaderImpl&) = delete;
    ZipReaderImpl(ZipReaderImpl&&) = delete;

    ZipReaderImpl& operator = (const ZipReaderImpl&) = delete;
    ZipReaderImpl& operator = (ZipReaderImpl&&) = delete;


    ~ZipReaderImpl() override {
        inflateEnd(&strm_);
    }

    [[nodiscard]] bool IsEof() const override { return done_; }

    void Finish() override {
        if (source_) {
            source_->Finish();
        }
    }

    [[nodiscard]] bool HaveMoreBufferedInput() const noexcept { return strm_.avail_in > 0; }

    boost::asio::const_buffers_1 ReadSome() override {

        size_t data_len = 0;

        while(!done_) {
            boost::string_ref src;
            if (HaveMoreBufferedInput()) {
                src = {};
            } else {
                const auto buffers = source_->ReadSome();
                src = {
                    boost::asio::buffer_cast<const char *>(buffers),
                    boost::asio::buffer_size(buffers)};

                if (src.empty()) {
                    throw DecompressException("Decompression failed - premature end of stream.");
                }
            }

            boost::string_ref out = {out_buffer_.data() + data_len,
                out_buffer_.size() - data_len};

            // Decompress sets leftover to cover unread input data
            Decompress(src, out);
            data_len += out.size();

            if ((out_buffer_.size() - data_len) == 0) {
                break;
            }
        }

        return {out_buffer_.data(), data_len};
    }

private:
    void Decompress(boost::string_ref& src,
                    boost::string_ref& dst) {

        RESTC_CPP_LOG_TRACE_("ZipReaderImpl::Decompress: " << src.size() << " bytes");

        if (!HaveMoreBufferedInput()) {
            strm_.next_in = const_cast<Bytef *>(
                reinterpret_cast<const Bytef *>(src.data()));
            strm_.avail_in
                = static_cast<decltype(strm_.avail_in)>(src.size());
        }

        assert(strm_.avail_in > 0);

        strm_.avail_out
            = static_cast<decltype(strm_.avail_out)>(dst.size());
        strm_.next_out = const_cast<Bytef *>(
            reinterpret_cast<const Bytef *>(dst.data()));

        assert(strm_.avail_out > 0);

        const auto result = inflate(&strm_, Z_SYNC_FLUSH);
        switch (result) {
            case Z_OK:
                break;
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            case Z_STREAM_ERROR: {
                std::string errmsg = "Decompression failed";
                if (strm_.msg != nullptr) {
                    errmsg += ": ";
                    errmsg += strm_.msg;
                }
                throw DecompressException(errmsg);
            }
            case Z_STREAM_END:
                RESTC_CPP_LOG_TRACE_("ZipReaderImpl::Decompress(): End Zstream. Done.");
                done_ = true;
                break;
            default: {
                std::string errmsg =
                    string("Decompression failed with unexpected value ")
                        + to_string(result);
                if (strm_.msg != nullptr) {
                    errmsg += ": ";
                    errmsg += strm_.msg;
                }
                throw DecompressException(errmsg);
            }
        }

        dst = {dst.data(), dst.size() - strm_.avail_out};
        RESTC_CPP_LOG_TRACE_("ZipReaderImpl::Decompress: src=" << dec << src.size() << " bytes, dst=" << dst.size() << " bytes");
    }

    unique_ptr<DataReader> source_;
    static constexpr size_t out_buffer_len_ = 1024*8;
    array<char, out_buffer_len_> out_buffer_ = {};
    z_stream strm_ = {};
    bool done_ = false;
};


std::unique_ptr<DataReader>
DataReader::CreateZipReader(std::unique_ptr<DataReader>&& source) {
    return make_unique<ZipReaderImpl>(std::move(source),
                                      ZipReaderImpl::Format::DEFLATE);
}

std::unique_ptr<DataReader>
DataReader::CreateGzipReader(std::unique_ptr<DataReader>&& source) {
    return make_unique<ZipReaderImpl>(std::move(source),
                                      ZipReaderImpl::Format::GZIP);
}

} // namepsace


