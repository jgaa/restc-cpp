#pragma once
#ifndef RESTC_CPP_H_
#define RESTC_CPP_H_

#include <string>
#include <map>
#include <memory>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/optional.hpp>

#include "restc-cpp/config.h"
#include "restc-cpp/helper.h"
#include "restc-cpp/Connection.h"

namespace restc_cpp {

class RestClient;
class Reply;
class Request;
class Connection;
class ConnectionPool;
class Socket;
class Request;
class Reply;
class Context;

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

    struct Body {
        enum class Type {
            NONE,
            STRING
        };

        Body() = default;
        Body(const std::string& body) : body_str_(body), type_{Type::STRING} {}
        Body(std::string&& body) : body_str_(move(body)), type_{Type::STRING} {}

        boost::optional<std::string> body_str_;
        const Type type_ = Type::NONE;
    };

    virtual const Properties& GetProperties() const = 0;
    virtual void SetProperties(Properties::ptr_t propreties) = 0;
    virtual std::unique_ptr<Reply> Execute(Context& ctx) = 0;

    static std::unique_ptr<Request>
    Create(const std::string& url,
           const Type requestType,
           RestClient& owner,
           boost::optional<Body> body = {},
           const boost::optional<args_t>& args = {},
           const boost::optional<headers_t>& headers = {});
};

class Reply {
public:

    static std::unique_ptr<Reply> Create(Connection::ptr_t& connection,
                                         Context& ctx,
                                         RestClient& owner);

    /*! Called after a request is sent to start interacting with the server.
     *
     * This will send the request to to the server and fetch the first
     * part of the reply, including the HTTP reply status and headers.
     *
     */
    virtual void StartReceiveFromServer() = 0;


    virtual int GetResponseCode() const = 0;

    /*! Get the complete data from the server and return it as a string.
     *
     * This is a convenience method when working with relatively
     * small results.
     */
    virtual std::string GetBodyAsString() = 0;

    /*! Get some data from the server.
     *
     * This is the lowest level to fetch data. Buffers will be
     * returned as they was returned from the web server.
     *
     * The method will return an empty buffer when there is
     * no more data to read (the request is complete).
     *
     * The data may be returned from a pending buffer, or it may
     * be fetched from the server.
     */
    virtual boost::asio::const_buffers_1 GetSomeData() = 0;

    /*! Returns true as ling as you have not yet pulled all
     * the data from the response.
     */
    virtual bool MoreDataToRead() = 0;

    /*! Get the value of a header */

    virtual boost::optional<std::string> GetHeader(const std::string& name) = 0;
};


class Context {
public:
    virtual boost::asio::yield_context& GetYield() = 0;
    virtual RestClient& GetClient() = 0;

    /*! Send a GET request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Get(std::string url) = 0;

    /*! Send a POST request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Post(std::string url, std::string body) = 0;

    /*! Send a PUT request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Put(std::string url, std::string body) = 0;

    /*! Send a DELETE request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Delete(std::string url) = 0;

    /*! Send a request asynchronously to the server.
     *
     * At the time the method returns, the request is sent, and the
     * first chunk of data is received from the server,. so that the
     * status code in the reply will ve valid.
     */
    virtual std::unique_ptr<Reply> Request(Request& req) = 0;
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
        Create(boost::optional<Request::Properties> properties = {});

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

