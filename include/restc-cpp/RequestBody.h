#pragma once

#ifndef RESTC_CPP_BODY_H_
#define RESTC_CPP_BODY_H_

#include "restc-cpp/DataWriter.h"

namespace restc_cpp {

/*! The body of the request. */
class RequestBody {
public:
    enum class Type {
        /// Typically a string or a file
        FIXED_SIZE,

        /// Use GetData() to pull for data
        CHUNKED_LAZY_PULL,

        /// User will push directly to the data writer
        CHUNKED_LAZY_PUSH,
    };

    virtual ~RequestBody() = default;

    virtual Type GetType() const noexcept = 0;

    /*! Typically the value of the content-length header */
    virtual std::uint64_t GetFixedSize() const = 0;

    /*! Returns true if we added data */
    virtual bool GetData(write_buffers_t& buffers) = 0;

    /*! Set the body up for a new run.
        *
        * This is typically done if request fails and the client wants
        * to re-try.
        */
    virtual void Reset() = 0;

    /*! Create a body with a string in it */
    static std::unique_ptr<RequestBody> CreateStringBody(std::string body);

    /*! Create a body from a file
     *
     * This will effectively upload the file.
     */
    static std::unique_ptr<RequestBody> CreateFileBody(
        boost::filesystem::path path);
};

} // restc_cpp

#endif // RESTC_CPP_BODY_H_
