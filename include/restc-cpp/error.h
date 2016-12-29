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
    : RequestFailedWithErrorException(response) { }
};

struct ParseException : public RestcCppException
{
    ParseException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct ProtocolException : public RestcCppException
{
    ProtocolException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct ConstraintException : public RestcCppException
{
    ConstraintException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct NotSupportedException : public RestcCppException
{
    NotSupportedException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct CommunicationException : public RestcCppException
{
    CommunicationException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct FailedToConnectException : public CommunicationException
{
    FailedToConnectException(const std::string& cause)
    : CommunicationException(cause) {}
};

struct DecompressException : public RestcCppException
{
    DecompressException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct NoDataException : public RestcCppException
{
    NoDataException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct CannotIncrementEndException : public RestcCppException
{
    CannotIncrementEndException(const std::string& cause)
    : RestcCppException(cause) {}
};


} // namespace
#endif // RESTC_CPP_ERROR_H_

