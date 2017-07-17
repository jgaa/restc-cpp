#pragma once

#ifndef RESTC_CPP_DATA_READER_H_
#define RESTC_CPP_DATA_READER_H_


#include <algorithm>

#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>

#include "restc-cpp.h"
#include "error.h"

namespace restc_cpp {

class Connection;
class Context;
class DataReaderStream;

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

    struct ReadConfig {
        int msReadTimeout = 0;
    };


    using ptr_t = std::unique_ptr<DataReader>;
    using add_header_fn_t = std::function<void(std::string&& name, std::string&& value)>;

    DataReader() = default;
    virtual ~DataReader() = default;

    virtual bool IsEof() const = 0;
    virtual boost::asio::const_buffers_1 ReadSome() = 0;

    static ptr_t CreateIoReader(const Connection::ptr_t& conn,
                                Context& ctx, const ReadConfig& cfg);
    static ptr_t CreateGzipReader(std::unique_ptr<DataReader>&& source);
    static ptr_t CreateZipReader(std::unique_ptr<DataReader>&& source);
    static ptr_t CreatePlainReader(size_t contentLength, ptr_t&& source);
    static ptr_t CreateChunkedReader(add_header_fn_t, std::unique_ptr<DataReaderStream>&& source);
    static ptr_t CreateNoBodyReader();
};

} // namespace

#endif // RESTC_CPP_DATA_READER_H_
