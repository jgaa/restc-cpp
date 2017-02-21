
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataWriter.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {


class PlainWriterImpl : public DataWriter {
public:
    PlainWriterImpl(size_t contentLength, ptr_t&& source)
    : next_{move(source)},  content_length_{contentLength}
    {
    }

    void WriteDirect(boost::asio::const_buffers_1 buffers) override {
        next_->WriteDirect(buffers);
    }

    void Write(boost::asio::const_buffers_1 buffers) override {
        next_->Write(buffers);
    }

    void Write(const write_buffers_t& buffers) override {
        next_->Write(buffers);
    }

    void Finish() override {
        next_->Finish();
    }

    void SetHeaders(Request::headers_t& headers) override {
        static const string content_length{"Content-Length"};
        headers[content_length] = to_string(content_length_);
        next_->SetHeaders(headers);
    }

private:
    unique_ptr<DataWriter> next_;
    const size_t content_length_;
};


DataWriter::ptr_t
DataWriter::CreatePlainWriter(size_t contentLength, ptr_t&& source) {
    return make_unique<PlainWriterImpl>(contentLength, move(source));
}

} // namespace


