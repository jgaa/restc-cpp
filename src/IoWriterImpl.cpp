
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataWriter.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {


class IoWriterImpl : public DataWriter {
public:
    IoWriterImpl(Connection& conn, Context& ctx)
    : ctx_{ctx}, connection_{conn}
    {
    }

    void Write(boost::asio::const_buffers_1 buffers) override {

        connection_.GetSocket().AsyncWrite(buffers, ctx_.GetYield());
        const auto bytes = boost::asio::buffer_size(buffers);

        RESTC_CPP_LOG_TRACE << "Wrote #" << bytes
            << " bytes to " << connection_;
    }

    void Write(const write_buffers_t& buffers) override {

        connection_.GetSocket().AsyncWrite(buffers, ctx_.GetYield());
        const auto bytes = boost::asio::buffer_size(buffers);

        RESTC_CPP_LOG_TRACE << "Wrote #" << bytes
            << " bytes to " << connection_;
    }

    void Finish() override {
        ;
    }

    void SetHeaders(Request::headers_t& ) override {
        ;
    }

private:
    Context& ctx_;
    Connection& connection_;
};



DataWriter::ptr_t
DataWriter::CreateIoWriter(Connection& conn, Context& ctx) {
    return make_unique<IoWriterImpl>(conn, ctx);
}

} // namespace


