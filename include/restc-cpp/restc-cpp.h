#pragma once
#ifndef RESTC_CPP_H_
#define RESTC_CPP_H_

#include "restc-cpp/config.h"

#include <string>
#include <map>
#include <deque>
#include <memory>
#include <future>
#include <fstream>
#include <iostream>
#include <array>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "restc-cpp/helper.h"
#include "restc-cpp/Connection.h"

#ifdef _MSC_VER
// Thank you Microsoft for making every developers day productive
#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif
#ifdef DELETE
#   undef DELETE
#endif
#endif

/*! Max expected data-payload for one server reply */
#ifndef RESTC_CPP_SANE_DATA_LIMIT
    // 16 Megabytes
#   define RESTC_CPP_SANE_DATA_LIMIT (1024 * 1024 * 16)
#endif

/*! Size of fixed size (per connection) IO buffers */
#ifndef RESTC_CPP_IO_BUFFER_SIZE
#   define RESTC_CPP_IO_BUFFER_SIZE (1024 * 16)
#endif

namespace restc_cpp {

class RestClient;
class Reply;
class Request;
class RequestBody;
class Connection;
class ConnectionPool;
class Socket;
class Request;
class Reply;
class Context;
class DataWriter;

using write_buffers_t = std::vector<boost::asio::const_buffer>;

class Request {
public:
    struct Arg {

        Arg() = default;
        Arg(const Arg&) = default;

        Arg(const std::string& use_name, const std::string& use_value)
        : name{use_name}, value{use_value} {}

        Arg(std::string&& use_name, std::string&& use_value)
        : name{std::move(use_name)}, value{std::move(use_value)} {}

        std::string name;
        std::string value;
    };

    struct Auth {
        Auth() = default;
        Auth(const Auth&) = default;
        Auth(Auth&&) = default;
        Auth(const std::string& authName, const std::string& authPasswd)
        : name{authName}, passwd{authPasswd} {}
        ~Auth() {
            std::memset(&name[0], 0, name.capacity());
            name.clear();
            std::memset(&passwd[0], 0, passwd.capacity());
            passwd.clear();
        }
        Auth& operator = (const Auth&) = default;
        Auth& operator = (Auth&&) = default;

        std::string name;
        std::string passwd;
    };

    struct Proxy {
        enum class Type { NONE, HTTP };
        Type type = Type::NONE;
        std::string address;
    };

    using headers_t = std::map<std::string, std::string, ciLessLibC>;
    using args_t = std::deque<Arg>;
    using auth_t = Auth;

    enum class Type {
        GET,
        POST,
        PUT,
        DELETE
    };

    class Properties {
    public:
        using ptr_t = std::shared_ptr<Properties>;

        int maxRedirects = 3;
        int connectTimeoutMs = (1000 * 12);
        int sendTimeoutMs = (1000 * 12); // For each IO operation
        int replyTimeoutMs =  (1000 * 21); // For each IO operation
        std::size_t cacheMaxConnectionsPerEndpoint = 16;
        std::size_t cacheMaxConnections = 128;
        int cacheTtlSeconds = 60;
        int cacheCleanupIntervalSeconds = 3;
        headers_t headers;
        args_t args;
        Proxy proxy;
    };

    virtual const Properties& GetProperties() const = 0;
    virtual void SetProperties(Properties::ptr_t propreties) = 0;
    virtual std::unique_ptr<Reply> Execute(Context& ctx) = 0;

    virtual ~Request() = default;

    static std::unique_ptr<Request>
    Create(const std::string& url,
           const Type requestType,
           RestClient& owner,
           std::unique_ptr<RequestBody> body = nullptr,
           const boost::optional<args_t>& args = {},
           const boost::optional<headers_t>& headers = {},
           const boost::optional<auth_t>& auth = {});
};

class Reply {
public:

    struct HttpResponse {
        enum class HttpVersion {
            HTTP_1_1
        };
        HttpVersion http_version = HttpVersion::HTTP_1_1;
        int status_code = 0;
        std::string reason_phrase;
    };

    virtual ~Reply() = default;

    /*! Get the unique ID for the connection */
    virtual boost::uuids::uuid GetConnectionId() const = 0;

    /*! Get the HTTP Response code received from the server */
    virtual int GetResponseCode() const = 0;

    /*! Get the HTTP Response received from the server */
    virtual const HttpResponse& GetHttpResponse() const = 0;

    /*! Get the complete data from the server and return it as a string.
     *
     * This is a convenience method when working with relatively
     * small results.
     */
    virtual std::string GetBodyAsString(size_t maxSize
        = RESTC_CPP_SANE_DATA_LIMIT) = 0;

    /*! Get some data from the server.
     *
     * This is the lowest level to fetch data. Buffers will be
     * returned as they was returned from the web server or
     * decompressed.
     *
     * The method will return an empty buffer when there is
     * no more data to read (the request is complete).
     *
     * The data may be returned from a pending buffer, or it may
     * be fetched from the server. The data is safe to use until
     * the method is called again.
     */
    virtual boost::asio::const_buffers_1 GetSomeData() = 0;

    /*! Returns true as long as you have not yet pulled all
     * the data from the response.
     */
    virtual bool MoreDataToRead() = 0;

    /*! Get the value of a header */
    virtual boost::optional<std::string>
        GetHeader(const std::string& name) = 0;
};

/*! The context is used to keep state within a co-routine.
 *
 * The Request and Reply classes depend on an instance of the
 * context to do their job.
 *
 * Internally, the Context is created in a functor referenced
 * by boost::asio::spawn().
 */
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
     * first chunk of data is received from the server. The status code
     * in the reply will be valid.
     */
    virtual std::unique_ptr<Reply> Request(Request& req) = 0;

    static std::unique_ptr<Context>
        Create(boost::asio::yield_context& yield,
               RestClient& rc);
};

/*! Factory and resource management
 *
 * Each instance of this class has it's own internal worker-thread
 * that will execute the co-routines passed to Process*().
 *
 * Because REST calls are typically slow at the server end, you can
 * normally pass a large number of requests to one instance.
 */
class RestClient {
public:
    using prc_fn_t = std::function<void (Context& ctx)>;
    struct DoneHandler {};

    /*! Get the default connection properties. */
    virtual const Request::Properties::ptr_t
        GetConnectionProperties() const = 0;
    virtual ~RestClient() = default;

    /*! Create a context and execute fn as a co-routine
     *
     * The REST operations in the coroutine are carried out by Request and
     * Reply. They will be executed asynchronously, and while an IO operation
     * is pending the co-routine will be suspended and the thread ready to
     * handle other co-routines.
     *
     * You should therefore not do time-consuming things within
     * fn, but rather execute long-running (more than a few milliseconds)
     * tasks in worker-threads. Do not wait for input or sleep() inside
     * the co-routine.
     */
    virtual void Process(const prc_fn_t& fn) = 0;

    /*! Process and return a future with a value or the current exception  */
    virtual std::future<void>
        ProcessWithPromise(const prc_fn_t& fn) = 0;

    /*! Process and return a future with a value or the current exception */
    template <typename T>
    std::future<T>
        ProcessWithPromiseT(const std::function<T (Context& ctx)>& fn) {

        auto prom = std::make_shared<std::promise<T>>();

        boost::asio::spawn(GetIoService(),
                           [prom,fn,this](boost::asio::yield_context yield) {
            auto ctx = Context::Create(yield, *this);
            auto done_handler = GetDoneHandler();
            try {
                prom->set_value(fn(*ctx));
            } catch(...) {
                prom->set_exception(std::current_exception());
            }
        });

        return prom->get_future();
    }


    virtual ConnectionPool& GetConnectionPool() = 0;
    virtual boost::asio::io_service& GetIoService() = 0;

    /*! Shut down the worker-thread when the work-queue is empty.

        \param wait Wait until the worker thread is shut down if true

     */
    virtual void CloseWhenReady(bool wait = true) = 0;

    /*! Factory */
    static std::unique_ptr<RestClient> Create();

    static std::unique_ptr<RestClient>
        Create(boost::optional<Request::Properties> properties);

    static std::unique_ptr<RestClient>
        Create(boost::optional<Request::Properties> properties,
            bool useMainThread);

    protected:
        virtual std::unique_ptr<DoneHandler> GetDoneHandler() = 0;
};


} // restc_cpp


#endif // RESTC_CPP_H_

