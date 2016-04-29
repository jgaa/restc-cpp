#pragma once
#ifndef RESTC_CPP_H_
#define RESTC_CPP_H_

#include <string>
#include <map>
#include <list>
#include <memory>
#include <future>
#include <fstream>
#include <iostream>
#include <array>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

#include "restc-cpp/config.h"
#include "restc-cpp/helper.h"
#include "restc-cpp/Connection.h"

#ifdef _MSC_VER
// Thank you Microsoft for making every developers day propductive
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

using write_buffers_t = std::vector<boost::asio::const_buffer>;

class Request {
public:
    struct Arg {

        Arg() = default;
        Arg(const Arg&) = default;
        Arg(const std::string& use_name, const std::string& use_value)
        : name{use_name}, value{use_value} {}

        std::string name;
        std::string value;
    };

    using headers_t = std::map<std::string, std::string, ciLessLibC>;
    using args_t = std::list<Arg>;

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
        headers_t headers;
        args_t args;
    };

    // TODO: Switch to interface and may be specialized implementations
    struct Body {
        enum class Type {
            NONE,
            STRING,
            FILE
        };

        Body() : eof_{true} {}
        Body(const std::string& body) : body_str_(body), type_{Type::STRING} {}
        Body(std::string&& body) : body_str_(move(body)), type_{Type::STRING} {}
        Body(const boost::filesystem::path& path) : path_(path), type_{Type::FILE} {}

        bool HaveSize() const noexcept {
            return true;
        }

        bool HaveAllDataReadyInBuffers() const noexcept {
            return type_ == Type::STRING;
        }

        bool IsEof() const noexcept {
            return eof_;
        }

        // Return true if we added data
        bool GetData(write_buffers_t& buffers);

        // Typically the value of the content-length header
        std::uint64_t GetFizxedSize() const;

        /*! Set the body up for a new run.
         *
         * This is typically done if request fails and the client wants
         * to re-try.
         */
        void Reset();

    private:
        boost::optional<std::string> body_str_;
        boost::optional<boost::filesystem::path> path_;
        const Type type_ = Type::NONE;
        bool eof_ = false;
        std::unique_ptr<std::ifstream> file_;
        std::unique_ptr<std::array<char, 1024 * 8>> buffer_;
        std::uint64_t bytes_read_ = 0;
        mutable boost::optional<std::uint64_t> size_;
    };

    virtual const Properties& GetProperties() const = 0;
    virtual void SetProperties(Properties::ptr_t propreties) = 0;
    virtual std::unique_ptr<Reply> Execute(Context& ctx) = 0;

    static std::unique_ptr<Request>
    Create(const std::string& url,
           const Type requestType,
           RestClient& owner,
           std::unique_ptr<Body> body = nullptr,
           const boost::optional<args_t>& args = {},
           const boost::optional<headers_t>& headers = {});
};

class Reply {
public:

    static std::unique_ptr<Reply> Create(Connection::ptr_t connection,
                                         Context& ctx,
                                         RestClient& owner);

    /*! Called after a request is sent to start interacting with the server.
     *
     * This will send the request to to the server and fetch the first
     * part of the reply, including the HTTP reply status and headers.
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
};

/*! Factory and resource management
 *
 * Each instance of this class has it's own internal worker-thread
 * that will execute the co-routines passed to Process().
 *
 * Because REST calls are typically slow at the server end, you can
 * normally pass a large number of requests to one instance.
 */
class RestClient {
public:
    using logger_t = std::function<void (const boost::string_ref)>;

    /*! Get the default connection properties. */
    virtual const Request::Properties::ptr_t GetConnectionProperties() const = 0;

    using prc_fn_t = std::function<void (Context& ctx)>;

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

    /*! Same as process, but returns a void future */
    virtual std::future<void> ProcessWithPromise(const prc_fn_t& fn) = 0;

    virtual ConnectionPool& GetConnectionPool() = 0;
    virtual boost::asio::io_service& GetIoService() = 0;

    /*! Factory */
    static std::unique_ptr<RestClient>
        Create(boost::optional<Request::Properties> properties = {});


    virtual void LogError(const boost::string_ref message) = 0;
    virtual void LogWarning(const boost::string_ref message) = 0;
    virtual void LogNotice(const boost::string_ref message) = 0;
    virtual void LogDebug(const boost::string_ref message) = 0;

    void LogError(const std::ostringstream& str) { LogError(str.str()); }
    void LogWarning(const std::ostringstream& str) { LogWarning(str.str()); }
    void LogNotice(const std::ostringstream& str) { LogNotice(str.str()); }
    void LogDebug(const std::ostringstream& str) { LogDebug(str.str()); }
};


} // restc_cpp


#endif // RESTC_CPP_H_

