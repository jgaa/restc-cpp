#pragma once

#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/steady_timer.hpp>

/**
 * @file
 * @brief Compatibility layer for handling breaking changes in Boost.Asio across versions.
 *
 * Boost frequently introduces breaking changes in its Asio library, with a backward
 * compatibility window of about 5 years. This header helps maintain compatibility with
 * multiple Boost versions, making it easier to support older versions without requiring
 * extensive refactoring.
 */

#if BOOST_VERSION >= 107000
#include <boost/coroutine/exceptions.hpp>
/// Macro for catching exceptions in Boost Coroutine, ensuring required handling for `forced_unwind`.
#define RESTC_CPP_IN_COROUTINE_CATCH_ALL \
catch (boost::coroutines::detail::forced_unwind const&) { \
        throw; /* required for Boost Coroutine! */ \
} catch (...)
#elif BOOST_VERSION >= 106000
#include <boost/coroutine2/detail/forced_unwind.hpp>
/// Macro for catching exceptions in Boost Coroutine, ensuring required handling for `forced_unwind`.
#define RESTC_CPP_IN_COROUTINE_CATCH_ALL \
catch (boost::coroutines::detail::forced_unwind const&) { \
        throw; /* required for Boost Coroutine! */ \
} catch (...)
#else
static_assert(false, "Unsupported boost version");
catch (...)
#endif

#if BOOST_VERSION >= 108100
/// Macro for handling function signature changes in Boost 1.86 and later.
#define RESTC_CPP_SPAWN_TRAILER \
    , boost::asio::detached
#else
#define RESTC_CPP_SPAWN_TRAILER
#endif

// This is insainity!
#if BOOST_VERSION >= 106700
#   define RESTC_CPP_STEADY_TIMER_EXPIRES_AFTER(ms) expires_after(ms)
#else
#   define RESTC_CPP_STEADY_TIMER_EXPIRES_AFTER(ms) expires_from_now(ms)
#endif

namespace restc_cpp {

#if BOOST_VERSION >= 107000
    /// Type alias for constant buffer in Boost 1.70 and later.
    using boost_const_buffer = boost::asio::const_buffer;
    /// Type alias for mutable buffer in Boost 1.70 and later.
    using boost_mutable_buffer = boost::asio::mutable_buffer;
#else
    /// Type alias for constant buffer in Boost versions earlier than 1.70.
    using boost_const_buffer = boost::asio::const_buffers_1;
    /// Type alias for mutable buffer in Boost versions earlier than 1.70.
    using boost_mutable_buffer = boost::asio::mutable_buffers_1;
#endif

#if BOOST_VERSION >= 106600
    /// Type alias for IO service in Boost 1.66 and later.
    using boost_io_service = boost::asio::io_context;
    /// Type alias for work guard in Boost 1.66 and later.
    using boost_work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
#else
    /// Type alias for IO service in Boost versions earlier than 1.66.
    using boost_io_service = boost::asio::io_service;
    /// Type alias for work guard in Boost versions earlier than 1.66.
    using boost_work = boost::asio::io_service::work;
#endif

    /**
 * @brief Extracts a const char pointer from a Boost buffer.
 *
 * @tparam Buffer The type of the buffer.
 * @param buffer The buffer to extract the pointer from.
 * @return A const char pointer to the data in the buffer.
 */
    template <typename Buffer>
    const char* boost_buffer_cast(const Buffer& buffer) {
#if BOOST_VERSION >= 107000
        return static_cast<const char*>(buffer.data());
#else
        return boost::asio::buffer_cast<const char*>(buffer);
#endif
    }

    /**
 * @brief Retrieves the size of a Boost buffer.
 *
 * @tparam Buffer The type of the buffer.
 * @param buffer The buffer to measure.
 * @return The size of the buffer in bytes.
 */
    template <typename Buffer>
    std::size_t boost_buffer_size(const Buffer& buffer) {
#if BOOST_VERSION >= 107000
        return buffer.size();
#else
        return boost::asio::buffer_size(buffer);
#endif
    }

    /**
 * @brief Dispatches a handler to the IO service.
 *
 * @tparam IOService The type of the IO service.
 * @tparam Handler The type of the handler.
 * @param io_service The IO service to use.
 * @param handler The handler to dispatch.
 */
    template <typename IOService, typename Handler>
    void boost_dispatch(IOService *io_service, Handler&& handler) {
#if BOOST_VERSION >= 106600
        io_service->get_executor().dispatch(
            std::forward<Handler>(handler),
            std::allocator<void>() // Default allocator
            );
#else
        io_service->dispatch(std::forward<Handler>(handler));
#endif
    }

    /**
 * @brief Wrapper for Boost resolver results for compatibility with older Boost versions.
 *
 * @tparam Iterator The type of the iterator used for results.
 */
    template <typename Iterator>
    class ResolverResultsWrapper {
    public:
        /**
     * @brief Constructor.
     * @param begin The beginning iterator of the results.
     * @param end The end iterator of the results.
     */
        explicit ResolverResultsWrapper(const Iterator& begin, const Iterator& end)
            : begin_(begin), end_(end) {}

        /**
     * @brief Returns the beginning iterator of the results.
     * @return The beginning iterator.
     */
        Iterator begin() const { return begin_; }

        /**
     * @brief Returns the end iterator of the results.
     * @return The end iterator.
     */
        Iterator end() const { return end_; }

    private:
        Iterator begin_;
        Iterator end_;
    };

    template <typename Resolver, typename YieldContext>
#if BOOST_VERSION >= 106600
    /// Type alias for resolver results in Boost 1.66 and later.
    using ResolverResults = boost::asio::ip::tcp::resolver::results_type;
#else
    /// Type alias for resolver results in Boost versions earlier than 1.66.
    using ResolverResults = ResolverResultsWrapper<boost::asio::ip::tcp::resolver::iterator>;
#endif

    /**
 * @brief Resolves a host and service to endpoints, with compatibility for multiple Boost versions.
 *
 * @tparam Resolver The type of the resolver.
 * @tparam YieldContext The type of the yield context.
 * @param resolver The resolver to use for the operation.
 * @param host The host to resolve.
 * @param service The service to resolve.
 * @param yield The yield context for asynchronous operations.
 * @return The resolver results, wrapped if necessary for older Boost versions.
 */
    template <typename Resolver, typename YieldContext>
    ResolverResults<Resolver, YieldContext> boost_resolve(
        Resolver& resolver,
        const std::string& host,
        const std::string& service,
        YieldContext yield)
    {
#if BOOST_VERSION >= 107000
        return resolver.async_resolve(host, service, yield);
#elif BOOST_VERSION >= 106600
        return resolver.async_resolve(host, service, yield);
#else
        boost::asio::ip::tcp::resolver::query query(host, service);
        auto it = resolver.async_resolve(query, yield);
        auto end = boost::asio::ip::tcp::resolver::iterator();
        return ResolverResultsWrapper(it, end);
#endif
    }

    /**
 * @brief Creates a Boost endpoint from an IP address and port.
 * @param ip_address The IP address as a string.
 * @param port The port number.
 * @return A Boost TCP endpoint.
 */
    boost::asio::ip::tcp::endpoint boost_create_endpoint(const std::string& ip_address, unsigned short port);

    /**
 * @brief Converts an IPv4 address from string format to a 32-bit unsigned integer.
 * @param ip_address The IPv4 address as a string.
 * @return The IPv4 address as a 32-bit unsigned integer.
 */
    uint32_t boost_convert_ipv4_to_uint(const std::string& ip_address);

    /**
 * @brief Creates a work guard for the given IO service.
 * @param ioservice The IO service to manage.
 * @return A unique pointer to the work guard.
 */
    std::unique_ptr<boost_work> boost_make_work(boost_io_service& ioservice);

} // namespace restc_cpp
