
#include <algorithm>

#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>

#include "restc-cpp.h"

namespace restc_cpp {

class Connection;
class Context;


/*! Generic IO interface to read data from the server.
 *
 * This interface allows us to add functionality, such as
 * chunked response handling and decompression simply by
 * inserting readers into the chain.
 *
 * Each reader will perform some specialized actions,
 * and fetch data from the next one when needed. The last
 * reader in the chain will be the one that reads data
 * from the network.
 */
class DataReader {
public:
    using ptr_t = std::unique_ptr<DataReader>;
    using add_header_fn_t = std::function<void(std::string&& name, std::string&& value)>;

    DataReader() = default;
    virtual ~DataReader() = default;

    virtual bool IsEof() const = 0;
    virtual boost::asio::const_buffers_1 ReadSome() = 0;

    static ptr_t CreateIoReader(Connection& conn, Context& ctx);
    static ptr_t CreateGzipReader(std::unique_ptr<DataReader>&& source);
    static ptr_t CreateZipReader(std::unique_ptr<DataReader>&& source);
    static ptr_t CreatePlainReader(size_t contentLength, ptr_t&& source);
    static ptr_t CreateChunkedReader(add_header_fn_t, ptr_t&& source);
    static ptr_t CreateNoBodyReader();
};

/*! Helper class for reading HTTP headers and chunk headers
 *
 * This class allows us to treat the data-source as both a stream
 * and buffer for maximum flexibility and performance.
 */
class DataReaderStream : public DataReader {
public:

    DataReaderStream(std::unique_ptr<DataReader>&& source)
    : source_{move(source)} {}

     bool IsEof() const {
         return eof_;
     }

    /*! Read whatever we have buffered or can get downstream */
    boost::asio::const_buffers_1 ReadSome() override;

    /*! Read up to maxBytes from whatever we have buffered or can get downstream.*/
    boost::asio::const_buffers_1 GetData(size_t maxBytes);

    /*! Get one char
     *
     * \exception runtime_error on end of stream
     */
    char Getc() {
        if (eof_) {
            throw std::runtime_error("Getc(): EOF");
        }

        Fetch();

        return *curr_;
    }

    void Ungetc() {
        assert(curr_ != nullptr);
        --curr_;
    }

    void SetEof() {
        eof_ = true;
    }

    char GetCurrentCh() const {
        assert(curr_ != nullptr);
        assert(curr_ <= end_);
        return *curr_;
    }

    /*! Read standard HTTP headers as a stream until we get an empty line
     *
     * The next buffer-position will be at the start of the body
     */
    void ReadServerResponse(Reply::HttpResponse& response);
    void ReadHeaderLines(const add_header_fn_t& addHeader);

private:
    void Fetch();
    std::string GetHeaderValue();

    bool eof_ = false;
    const char *curr_ = nullptr;
    const char *end_ = nullptr;
    DataReader::ptr_t source_;
    size_t num_headers_ = 0;
};

} // namespace
