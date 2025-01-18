#pragma once

#include <boost/asio.hpp>
#include <boost/version.hpp>
#include <boost/asio/ip/address_v4.hpp>

/* Boost keeps introducing breaking changes in the asio library. It seems like
 * their window of backwards compatibility is about 5 years.
 *
 * This is a nightmare for library developers, if we want to maintain support
 * for older versions of boost. I don't want the users of restc-cpp to be forced to
 * refactor their code just because the boost library has been updated. Refactoring
 * for this reason alone is just a waste of time and a huge cost for no gain.
 *
 * Here I try to handle the differences between the different versions of boost::asio
 * in order to make it easier to maintain support for older versions of boost.
 *
 * So if restc-cpp is the only library that you are using that requires broken parts
 * of boost, then you should be fine.
 *
 * I take full credits for whatever works well here. All blame goes to ChatGPT! ;)
 */

#if BOOST_VERSION >= 107000
#include <boost/coroutine/exceptions.hpp>
#define RESTC_CPP_IN_COROUTINE_CATCH_ALL \
catch (boost::coroutines::detail::forced_unwind const&) { \
        throw; /* required for Boost Coroutine! */ \
} catch (...)
#elif BOOST_VERSION >= 106000
#include <boost/coroutine2/detail/forced_unwind.hpp>
#define RESTC_CPP_IN_COROUTINE_CATCH_ALL \
catch (boost::coroutines::detail::forced_unwind const&) { \
        throw; /* required for Boost Coroutine! */ \
} catch (...)
#else
static_assert(false, "Unsupported boost version");
catch (...)
#endif

#if BOOST_VERSION >= 108100
// They changed the function signature. In boost 1.86 it broke the build.
#define RESTC_CPP_SPAWN_TRAILER \
, boost::asio::detached
#else
#define RESTC_CPP_SPAWN_TRAILER
#endif


namespace restc_cpp {

#if BOOST_VERSION >= 107000
using boost_const_buffer = boost::asio::const_buffer;
using boost_mutable_buffer = boost::asio::mutable_buffer;
#else
using boost_const_buffer = boost::asio::const_buffers_1;
using boost_mutable_buffer = boost::asio::mutable_buffers_1;
#endif


#if BOOST_VERSION >= 106600
using boost_io_service = boost::asio::io_context;
using boost_work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

#else
using boost_io_service = boost::asio::io_service;
using boost_work = boost::asio::io_service::work;
#endif

template <typename Buffer>
const char* boost_buffer_cast(const Buffer& buffer) {
#if BOOST_VERSION >= 107000
    return static_cast<const char*>(buffer.data());
#else
    return boost::asio::buffer_cast<const char*>(buffer);
#endif
}

template <typename Buffer>
std::size_t boost_buffer_size(const Buffer& buffer) {
#if BOOST_VERSION >= 107000
    return buffer.size();
#else
    return boost::asio::buffer_size(buffer);
#endif
}

template <typename IOService, typename Handler>
void boost_dispatch(IOService& io_service, Handler&& handler) {
#if BOOST_VERSION >= 106600
    // Determine if IOService is a pointer
    if constexpr (std::is_pointer_v<IOService>) {
        io_service->get_executor().dispatch(
            std::forward<Handler>(handler),
            std::allocator<void>() // Default allocator
            );
    } else {
        io_service.get_executor().dispatch(
            std::forward<Handler>(handler),
            std::allocator<void>() // Default allocator
            );
    }
#else
    if constexpr (std::is_pointer_v<IOService>) {
        io_service->dispatch(std::forward<Handler>(handler));
    } else {
        io_service.dispatch(std::forward<Handler>(handler));
    }
#endif
}

boost::asio::ip::tcp::endpoint boost_create_endpoint(const std::string& ip_address, unsigned short port);
uint32_t boost_convert_ipv4_to_uint(const std::string& ip_address);
std::unique_ptr<boost_work> boost_make_work(boost_io_service& ioservice);

} // ns


