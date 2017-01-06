#pragma once

#ifndef RESTC_CPP_DATA_READER_STREAM_H_
#define RESTC_CPP_DATA_READER_STREAM_H_


#include <algorithm>

#include <boost/asio.hpp>
#include <boost/utility/string_ref.hpp>

#include "restc-cpp.h"
#include "DataReader.h"

namespace restc_cpp {

/*! Helper class for reading HTTP headers and chunk headers
 *
 * This class allows us to treat the data-source as both a stream
 * and buffer for maximum flexibility and performance.
 */
class DataReaderStream : public DataReader {
public:

    DataReaderStream(std::unique_ptr<DataReader>&& source)
    : source_{move(source)} {}

     bool IsEof() const override {
         return eof_;
     }

    /*! Read whatever we have buffered or can get downstream */
    boost::asio::const_buffers_1 ReadSome() override;

    /*! Read up to maxBytes from whatever we have buffered or can get downstream.*/
    boost::asio::const_buffers_1 GetData(size_t maxBytes);

    /*! Get one char
     *
     * \exception ParseException on end of stream
     */
    char Getc() {
        if (eof_) {
            throw ParseException("Getc(): EOF");
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

#endif // RESTC_CPP_DATA_READER_STREAM_H_

