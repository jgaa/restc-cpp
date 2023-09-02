
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataWriter.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/IoTimer.h"

using namespace std;

namespace restc_cpp {


class IoWriterImpl : public DataWriter {
public:
    IoWriterImpl(const Connection::ptr_t& conn, Context& ctx,
                 const WriteConfig& cfg)
    : ctx_{ctx}, cfg_{cfg}, connection_{conn}
    {
    }

    void WriteDirect(boost::asio::const_buffers_1 buffers) override {
        Write(buffers);
    }

    void Write(boost::asio::const_buffers_1 buffers) override {

        {
            auto timer = IoTimer::Create("IoWriterImpl",
                                        cfg_.msWriteTimeout,
                                        connection_);

            connection_->GetSocket().AsyncWrite(buffers, ctx_.GetYield());
        }

        RESTC_CPP_LOG_TRACE_("Wrote #" << boost::asio::buffer_size(buffers)
            << " bytes to " << connection_);
    }

    void Write(const write_buffers_t& buffers) override {

        {
            auto timer = IoTimer::Create("IoWriterImpl",
                                        cfg_.msWriteTimeout,
                                        connection_);

            connection_->GetSocket().AsyncWrite(buffers, ctx_.GetYield());
        }

        RESTC_CPP_LOG_TRACE_("Wrote #" << boost::asio::buffer_size(buffers)
            << " bytes to " << connection_);
    }

    void Finish() override {
        ;
    }

    void SetHeaders(Request::headers_t& ) override {
        ;
    }

private:
    Context& ctx_;
    WriteConfig cfg_;
    const Connection::ptr_t& connection_;
};



DataWriter::ptr_t
DataWriter::CreateIoWriter(const Connection::ptr_t& conn, Context& ctx,
                           const WriteConfig& cfg) {
    return make_unique<IoWriterImpl>(conn, ctx, cfg);
}

} // namespace


