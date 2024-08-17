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

    void Finish() override {
    }


    boost::asio::const_buffers_1 ReadSome() override {
        if (auto conn = connection_.lock()) {
            auto timer = IoTimer::Create("IoReaderImpl",
                                        cfg_.msReadTimeout,
                                        conn);

            for(size_t retries = 0;; ++retries) {
                size_t bytes = 0;
                try {
                    if (retries != 0u) {
                        RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: taking a nap");
                        ctx_.Sleep(retries * 20ms);
                        RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: Waking up. Will try to read from the socket now.");
                    }

                    bytes = conn->GetSocket().AsyncReadSome(
                        {buffer_.data(), buffer_.size()}, ctx_.GetYield());
                } catch (const boost::system::system_error& ex) {
                    if (ex.code() == boost::system::errc::resource_unavailable_try_again) {
                        if ( retries < 32) {
                            RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: Caught boost::system::system_error exception: " << ex.what()
                                                 << ". I will continue the retry loop.");
                            continue;
                        }
                    }
                    RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: Caught boost::system::system_error exception: " << ex.what());
                    throw;
                } catch (const exception& ex) {
                    RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: Caught exception: " << ex.what());
                    throw;
                }

                RESTC_CPP_LOG_TRACE_("Read #" << bytes
                    << " bytes from " << conn);

                timer->Cancel();
                return {buffer_.data(), bytes};
            }
        }

        RESTC_CPP_LOG_DEBUG_("IoReaderImpl::ReadSome: Reached outer scope. Timed out?");
        throw ObjectExpiredException("Connection expired");
    }

    [[nodiscard]] bool IsEof() const override
    {
        if (auto conn = connection_.lock()) {
            return !conn->GetSocket().IsOpen();
        }
        return true;
    }

private:
    Context& ctx_;
    const std::weak_ptr<Connection> connection_;
    const ReadConfig cfg_;
    buffer_t buffer_ = {};
};



DataReader::ptr_t
DataReader::CreateIoReader(const Connection::ptr_t& conn, Context& ctx,
                           const DataReader::ReadConfig& cfg) {
    return make_unique<IoReaderImpl>(conn, ctx, cfg);
}

} // namespace


