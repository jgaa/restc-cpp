
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "DataReader.h"

using namespace std;

namespace restc_cpp {

class IoReaderImpl : public DataReader {
public:
    IoReaderImpl(Connection& conn, Context& ctx)
    : ctx_{ctx}, connection_{conn}
    {
    }

    size_t ReadSome(boost::asio::mutable_buffers_1 read_buffers) override {
        return connection_.GetSocket().AsyncReadSome(read_buffers,
                                                     ctx_.GetYield());
    }

private:
    Context& ctx_;
    Connection& connection_;
};

std::unique_ptr<DataReader>
DataReader::CreateIoReader(Connection& conn, Context& ctx) {
    return make_unique<IoReaderImpl>(conn, ctx);
}

} // namepsace
