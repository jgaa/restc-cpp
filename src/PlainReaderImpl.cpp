#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/error.h"

using namespace std;

namespace restc_cpp {

class PlainReaderImpl : public DataReader {
public:

    PlainReaderImpl(size_t contentLength, ptr_t&& source)
    : remaining_{contentLength},
      source_{move(source)} {}

    [[nodiscard]] bool IsEof() const override { return remaining_ == 0; }

    void Finish() override {
        if (source_) {
            source_->Finish();
        }
    }

    boost::asio::const_buffers_1 ReadSome() override {

        if (IsEof()) {
            return {nullptr, 0};
        }

        auto buffer = source_->ReadSome();
        const auto bytes = boost::asio::buffer_size(buffer);

        if ((static_cast<int64_t>(remaining_)
            - static_cast<int64_t>(bytes)) < 0) {
            throw ProtocolException("Body-size exceeds content-size");
        }
        remaining_ -= bytes;
        return buffer;
    }

private:
    size_t remaining_;
    ptr_t source_;
};

DataReader::ptr_t
DataReader::CreatePlainReader(size_t contentLength, ptr_t&& source) {
    return make_unique<PlainReaderImpl>(contentLength, move(source));
}


} // namespace
