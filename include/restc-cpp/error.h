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

struct HttpAuthenticationException: public RequestFailedWithErrorException {

    HttpAuthenticationException(const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response) { }
};

struct HttpForbiddenException: public RequestFailedWithErrorException {

    HttpForbiddenException(const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response) { }
};

struct HttpNotFoundException: public RequestFailedWithErrorException {

    HttpNotFoundException(const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response) { }
};

struct HttpMethodNotAllowedException: public RequestFailedWithErrorException {

    HttpMethodNotAllowedException(const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response) { }
};

struct HttpNotAcceptableException: public RequestFailedWithErrorException {

    HttpNotAcceptableException(const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response) { }
};

struct HttpProxyAuthenticationRequiredException
: public RequestFailedWithErrorException {

    HttpProxyAuthenticationRequiredException(
        const Reply::HttpResponse& response)
    : RequestFailedWithErrorException(response) { }
};

struct HttpRequestTimeOutException: public RequestFailedWithErrorException {

    HttpRequestTimeOutException(const Reply::HttpResponse& response)
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

struct NotImplementedException : public RestcCppException
{
    NotImplementedException(const std::string& cause)
    : RestcCppException(cause) {}
};

struct IoException : public RestcCppException
{
    IoException(const std::string& cause)
    : RestcCppException(cause) {}
};


} // namespace
#endif // RESTC_CPP_ERROR_H_

