
#include <boost/asio.hpp>

namespace restc_cpp {

class Connection;
class Context;

class DataReader {
public:
    virtual ~DataReader() = default;

    //virtual bool IsEof() const = 0;
    virtual size_t ReadSome(boost::asio::mutable_buffers_1 read_buffers) = 0;

    static std::unique_ptr<DataReader>
    CreateIoReader(Connection& conn, Context& ctx);

};

} // namespace
