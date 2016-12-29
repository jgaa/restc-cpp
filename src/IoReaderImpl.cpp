#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataReader.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {


class IoReaderImpl : public DataReader {
public:
    using buffer_t = std::array<char, 1024 * 16>;

    IoReaderImpl(Connection& conn, Context& ctx)
    : ctx_{ctx}, connection_{conn}
    {
    }

    boost::asio::const_buffers_1 ReadSome() override {
        const auto bytes = connection_.GetSocket().AsyncReadSome(
            {buffer_.data(), buffer_.size()}, ctx_.GetYield());
        
        RESTC_CPP_LOG_TRACE << "Read #" << bytes << " bytes from " << connection_;
        return {buffer_.data(), bytes};
    }

     bool IsEof() const override {
         return !connection_.GetSocket().IsOpen();
     }

private:
    Context& ctx_;
    Connection& connection_;
    buffer_t buffer_;
};



DataReader::ptr_t
DataReader::CreateIoReader(Connection& conn, Context& ctx) {
    return make_unique<IoReaderImpl>(conn, ctx);
}



} // namespace


