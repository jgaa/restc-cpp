#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/IoTimer.h"

using namespace std;

namespace restc_cpp {


class IoReaderImpl : public DataReader {
public:
    using buffer_t = std::array<char, RESTC_CPP_IO_BUFFER_SIZE>;

    IoReaderImpl(const Connection::ptr_t& conn, Context& ctx,
                 const ReadConfig& cfg)
    : ctx_{ctx}, connection_{conn}, cfg_{cfg}
    {
    }

    boost::asio::const_buffers_1 ReadSome() override {
        auto timer = IoTimer::Create("IoReaderImpl",
                                     cfg_.msReadTimeout,
                                     connection_);
        const auto bytes = connection_->GetSocket().AsyncReadSome(
            {buffer_.data(), buffer_.size()}, ctx_.GetYield());

        timer->Cancel();

        RESTC_CPP_LOG_TRACE << "Read #" << bytes
            << " bytes from " << connection_;
        return {buffer_.data(), bytes};
    }

    bool IsEof() const override {
        return !connection_->GetSocket().IsOpen();
    }

private:
    Context& ctx_;
    const Connection::ptr_t connection_;
    const ReadConfig cfg_;
    buffer_t buffer_;
};



DataReader::ptr_t
DataReader::CreateIoReader(const Connection::ptr_t& conn, Context& ctx,
                           const DataReader::ReadConfig& cfg) {
    return make_unique<IoReaderImpl>(conn, ctx, cfg);
}

} // namespace


