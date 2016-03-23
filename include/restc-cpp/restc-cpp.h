#pragma once
#ifndef RESTC_CPP_H_
#define RESTC_CPP_H_

/* This is a minimalistic but still functioning and very fast
 * HTTP/HTTPS client library in C++.
 *
 * It depends on C++14 with its standard libraries and boost.
 * It uses boost::asio for IO.
 *
 * The library is written by Jarle (jgaa) Aase, an enthusiastic
 * C++ software developer since 1996. (Before that, I used C).
 *
 * The design goal of this project is to make external REST API's
 * simple to use in C++ projects, but still very fast (which is why
 * we use C++ in the first place, right?). Since it uses boost::asio,
 * it's a perfect match for client libraries and applications,
 * but also modern, powerful C++ servers, since these more and more
 * defaults to boost:asio for network IO.
 *
 * Usually I use some version of GPL or LGPL for my projects. This
 * library however is so tiny and general that I have released it
 * under the more permissive MIT license.
 *
 * Supported development platforms:
 *  - Linux (Debian stable and testing)
 *  - Windows 10 (Latest "community" C++ compiler from Microsoft
 *
 * Suggested target platforms:
 *  - Linux
 *  - OS/X
 *  - Android (via NDK)
 *  - Windows Vista and later
 *  - Windows mobile
 */


#include <string>
#include <map>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "restc-cpp/helper.h"

namespace restc_cpp {

class RestClient;
class Reply;
class Request;
class Connection;

class Context {
public:
    virtual boost::asio::yield_context& GetYield() = 0;
    virtual RestClient& GetClient() = 0;

    virtual std::unique_ptr<Reply> Get(std::string url) = 0;
    virtual std::unique_ptr<Reply> Request(Request& req) = 0;
};


class Request {
public:
    using headers_t = std::map<std::string, std::string, ciLessLibC>;
    using args_t = std::map<std::string, std::string>;

    enum class Type {
        GET,
        POST,
        PUT,
        DELETE,
    };

    class Properties {
    public:

        using ptr_t = std::shared_ptr<Properties>;

        int maxRedirects = 3;
        int connectTimeoutMs = (1000 * 12);
        int replyTimeoutMs =  (1000 * 60);
        headers_t headers;
        args_t args;
    };


    virtual const Properties& GetProperties() const = 0;
    virtual void SetProperties(Properties::ptr_t propreties) = 0;
    virtual std::unique_ptr<Reply> Execute(Context& ctx) = 0;

    static std::unique_ptr<Request>
    Create(const std::string& url,
           const Type requestType,
           RestClient& owner,
           const args_t *args = nullptr,
           const headers_t *headers = nullptr);
};

class Reply {
public:

    static std::unique_ptr<Reply> Create(Context& ctx,
                                         const Request& req,
                                         RestClient& owner);
};


class Socket
{
public:
    virtual ~Socket() = default;

    virtual boost::asio::ip::tcp::socket& GetSocket() = 0;

    virtual const boost::asio::ip::tcp::socket& GetSocket() const = 0;

    virtual std::size_t AsyncReadSome(boost::asio::mutable_buffers_1 buffers,
                                      boost::asio::yield_context& yield) = 0;

    virtual std::size_t AsyncRead(boost::asio::mutable_buffers_1 buffers,
                                  boost::asio::yield_context& yield) = 0;

    virtual void AsyncWrite(const boost::asio::const_buffers_1& buffers,
                            boost::asio::yield_context& yield) = 0;

    virtual void AsyncConnect(const boost::asio::ip::tcp::endpoint& ep,
                              boost::asio::yield_context& yield) = 0;

    virtual void AsyncShutdown(boost::asio::yield_context& yield) = 0;

};


class Connection {
public:
    using ptr_t = std::unique_ptr<Connection>;
    using release_callback_t = std::function<void (Connection&)>;

    enum class Type {
        HTTP,
        HTTPS
    };

    virtual ~Connection() = default;

    virtual Socket& GetSocket() = 0;
};

class ConnectionPool {
public:
    virtual Connection::ptr_t GetConnection(
        const boost::asio::ip::tcp::endpoint ep,
        const Connection::Type connectionType,
        bool new_connection_please = false) = 0;

    static std::unique_ptr<ConnectionPool> Create(RestClient& owner);
};

/*! Factory and resource management
*/
class RestClient {
public:
    using logger_t = std::function<void (const char *)>;

    /*! Get the default connection properties. */
    virtual const Request::Properties::ptr_t GetConnectionProperties() const = 0;

    using prc_fn_t = std::function<void (Context& ctx)>;

    virtual void Process(const prc_fn_t& fn) = 0;

    virtual ConnectionPool& GetConnectionPool() = 0;
    virtual boost::asio::io_service& GetIoService() = 0;

    /*! Factory */
    static std::unique_ptr<RestClient>
        Create(Request::Properties *properties = nullptr);

    virtual void LogError(const char *message) = 0;
    virtual void LogWarning(const char *message) = 0;
    virtual void LogNotice(const char *message) = 0;
    virtual void LogDebug(const char *message) = 0;

    void LogError(const std::ostringstream& str) { LogError(str.str().c_str()); }
    void LogWarning(const std::ostringstream& str) { LogWarning(str.str().c_str()); }
    void LogNotice(const std::ostringstream& str) { LogNotice(str.str().c_str()); }
    void LogDebug(const std::ostringstream& str) { LogDebug(str.str().c_str()); }

    void LogError(const std::string& str) { LogError(str.c_str()); }
    void LogWarning(const std::string& str) { LogWarning(str.c_str()); }
    void LogNotice(const std::string& str) { LogNotice(str.c_str()); }
    void LogDebug(const std::string& str) { LogDebug(str.c_str()); }
};


} // restc_cpp


#endif // RESTC_CPP_H_

