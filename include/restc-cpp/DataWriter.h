#pragma once

#ifndef RESTC_CPP_DATA_WRITER_H_
#define RESTC_CPP_DATA_WRITER_H_


#include <algorithm>

#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>

#include "restc-cpp.h"
#include "error.h"

namespace restc_cpp {

class Connection;
class Context;


/*! Generic IO interface to write data from the server.
 *
 * This interface allows us to add functionality, such as
 * chunked response handling and decompression simply by
 * inserting writers into the chain.
 *
 * Each writer will perform some specialized actions,
 * and write data to the next one when required. The last
 * writer in the chain will be the one that writes data
 * to the network.
 */
class DataWriter {
public:
    using ptr_t = std::unique_ptr<DataWriter>;

    /*! Allows the user to set the headers in the chunked trailer if required */
    using add_header_fn_t = std::function<std::string()>;

    DataWriter() = default;
    virtual ~DataWriter() = default;

    /*! Write some data */
    virtual void Write(boost::asio::const_buffers_1 buffers) = 0;

    /*! Write without altering the data (headers) */
    virtual void WriteDirect(boost::asio::const_buffers_1 buffers) = 0;

    /*! Write some data */
    virtual void Write(const write_buffers_t& buffers) = 0;

    /*! Called when all data is written to flush the buffers */
    virtual void Finish() = 0;

    /*! Set the headers required by the writer.
     *
     * For example, the plain data writer will set the content-length,
     * while the chunked writer will set the header for chunked
     * data.
     */
    virtual void SetHeaders(Request::headers_t& headers) = 0;

    static ptr_t CreateIoWriter(Connection& conn, Context& ctx);
    static ptr_t CreateGzipWriter(std::unique_ptr<DataWriter>&& source);
    static ptr_t CreateZipWriter(std::unique_ptr<DataWriter>&& source);
    static ptr_t CreatePlainWriter(size_t contentLength, ptr_t&& source);
    static ptr_t CreateChunkedWriter(add_header_fn_t, ptr_t&& source);
    static ptr_t CreateNoBodyWriter();
};

} // namespace

#endif // RESTC_CPP_DATA_WRITER_H_
