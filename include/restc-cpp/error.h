#pragma once
#ifndef RESTC_CPP_ERROR_H_
#define RESTC_CPP_ERROR_H_

#include "restc-cpp.h"

namespace restc_cpp {

struct RestcCppException : public std::runtime_error {

    RestcCppException(const std::string& cause)
    : runtime_error(cause) {}
};

struct RequestFailedWithErrorException : public RestcCppException {

    RequestFailedWithErrorException(const Reply::HttpResponse& response)
    : RestcCppException(std::string("Request failed with HTTP error: ")
        + std::to_string(response.status_code)
        + " " + response.reason_phrase)
    , http_response{response}
    {
    }

    const Reply::HttpResponse http_response;
};

struct AuthenticationException: public RequestFailedWithErrorException {

    AuthenticationException(const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response)
    {
    }
};

} // namespace
#endif // RESTC_CPP_ERROR_H_

