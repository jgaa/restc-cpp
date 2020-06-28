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

#ifdef RESTC_CPP_WITH_TLS
#   include <boost/asio/ssl.hpp>
#endif

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "restc-cpp/helper.h"
#include "restc-cpp/Connection.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
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

/*! Length of lines when we 'pretty-print' */
constexpr size_t line_length = 80;

using write_buffers_t = std::vector<boost::asio::const_buffer>;

struct Headers : public std::multimap<std::string, std::string, ciLessLibC>  {

    /*! Operate on headers that can only have one instance per key */
    std::string& operator[] (const std::string& key) {
        auto it = find(key);
        if (it != end()) {
            return it->second;
        }

        return insert({key, {}})->second;
    }
};

using headers_t = Headers;

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

    using args_t = std::deque<Arg>;
    using auth_t = Auth;
    using headers_t = restc_cpp::headers_t;

    enum class Type {
        GET,
        POST,
        PUT,
        DELETE,
        OPTIONS,
        HEAD,
        PATCH
    };

    class Properties {
    public:
        using ptr_t = std::shared_ptr<Properties>;
        using redirect_fn_t = std::function<void (int code, std::string& url, 
                                                  const Reply& reply)>;

        int maxRedirects = 3;
        int connectTimeoutMs = (1000 * 12);
        int sendTimeoutMs = (1000 * 12); // For each IO operation
        int replyTimeoutMs =  (1000 * 21); // For the reply header
        int recvTimeout = (1000 * 21); // For each IO operation
        std::size_t cacheMaxConnectionsPerEndpoint = 16;
        std::size_t cacheMaxConnections = 128;
        int cacheTtlSeconds = 60;
        int cacheCleanupIntervalSeconds = 3;
        headers_t headers;
        args_t args;
        Proxy proxy;
        redirect_fn_t redirectFn;
    };

    virtual const Properties& GetProperties() const = 0;
    virtual void SetProperties(Properties::ptr_t propreties) = 0;

    /*! Manually send the request */
    virtual DataWriter& SendRequest(Context& ctx) = 0;

    /*! Call after SendRequest
     *
     * This completes the request stage, and fetches the reply
     * from the server.
     *
     * \Note if you call SendRequest() and GetReply() manually,
     *      you have to deal with redirects yourself.
     *
     * \See RedirectException
     */
    virtual std::unique_ptr<Reply> GetReply(Context& ctx) = 0;

    /*! Execute the request
     *
     * This method calls SendRequest() and GetReply() and
     * deals with redirects, according to the settings
     * in the properties.
     */
    virtual std::unique_ptr<Reply> Execute(Context& ctx) = 0;

    virtual ~Request() = default;

    static std::unique_ptr<Request>
    Create(const std::string& url,
           const Type requestType,
           RestClient& owner,
           std::unique_ptr<RequestBody> body = {},
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

    /*! Get the values from multiple headers with the same name */
    virtual std::deque<std::string> GetHeaders(const std::string& name) = 0;
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

    /*! Send a OPTIONS request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Options(std::string url) = 0;

    /*! Send a HEAD request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Head(std::string url) = 0;

    /*! Send a PATCH request asynchronously to the server. */
    virtual std::unique_ptr<Reply> Patch(std::string url) = 0;

    /*! Send a request asynchronously to the server.
     *
     * At the time the method returns, the request is sent, and the
     * first chunk of data is received from the server. The status code
     * in the reply will be valid.
     */
    virtual std::unique_ptr<Reply> Request(Request& req) = 0;

    /*! Asynchronously sleep for a period */
    template<class Rep, class Period>
    void Sleep(const std::chrono::duration<Rep, Period>& duration) {
        const auto microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(
                duration).count();
        boost::posix_time::microseconds ms(microseconds);
        Sleep(ms);
    }

    /*! Asynchronously sleep for a period */
    virtual void Sleep(const boost::posix_time::microseconds& ms) = 0;

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
    class DoneHandler
    {
    public:
        DoneHandler() = default;
        virtual ~DoneHandler() = default;
    };

    /*! Get the default connection properties. */
    virtual Request::Properties::ptr_t GetConnectionProperties() const = 0;
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
        auto future = prom->get_future();

        boost::asio::spawn(GetIoService(),
                           [prom,fn,this](boost::asio::yield_context yield) {
            auto ctx = Context::Create(yield, *this);
            auto done_handler = GetDoneHandler();
            try {
                prom->set_value(fn(*ctx));
            } catch(...) {
                prom->set_exception(std::current_exception());
            }
            done_handler.reset();
        });

        return move(future);
    }

    /*! Process from within an existing coroutine */
    template <typename T>
    T ProcessWithYield(const std::function<T (Context& ctx)>& fn, boost::asio::yield_context& yield) {
        auto ctx = Context::Create(yield, *this);
        fn(yield);
    }

    virtual std::shared_ptr<ConnectionPool> GetConnectionPool() = 0;
    virtual boost::asio::io_service& GetIoService() = 0;

#ifdef RESTC_CPP_WITH_TLS
    virtual std::shared_ptr<boost::asio::ssl::context> GetTLSContext() = 0;
#endif

    /*! Shut down the worker-thread when the work-queue is empty.

        \param wait Wait until the worker thread is shut down if true

     */
    virtual void CloseWhenReady(bool wait = true) = 0;

    /*! Factory */
    static std::unique_ptr<RestClient> Create();

#ifdef RESTC_CPP_WITH_TLS
	static std::unique_ptr<RestClient> Create(std::shared_ptr<boost::asio::ssl::context> ctx);
#endif

    static std::unique_ptr<RestClient>
        Create(const boost::optional<Request::Properties>& properties);

    static std::unique_ptr<RestClient> CreateUseOwnThread();

    static std::unique_ptr<RestClient>
        CreateUseOwnThread(const boost::optional<Request::Properties>& properties);

    static std::unique_ptr<RestClient>
        Create(const boost::optional<Request::Properties>& properties,
               boost::asio::io_service& ioservice);

    static std::unique_ptr<RestClient>
        Create(boost::asio::io_service& ioservice);

    protected:
        virtual std::unique_ptr<DoneHandler> GetDoneHandler() = 0;
};


} // restc_cpp


#endif // RESTC_CPP_H_

