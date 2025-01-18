
#include "restc-cpp/boost_compatibility.h"

namespace restc_cpp {
boost::asio::ip::tcp::endpoint boost_create_endpoint(const std::string& ip_address, unsigned short port) {
#if BOOST_VERSION >= 106600
    // For Boost 1.66.0 and later
    return {boost::asio::ip::make_address(ip_address), port};
#else
    // For Boost versions earlier than 1.66.0
    return {boost::asio::ip::address::from_string(ip_address), port};
#endif
}

uint32_t boost_convert_ipv4_to_uint(const std::string& ip_address) {
#if BOOST_VERSION >= 106600
    // For Boost 1.66.0 and later
    return boost::asio::ip::make_address_v4(ip_address).to_uint();
#else
    // For Boost versions earlier than 1.66.0
    return boost::asio::ip::address_v4::from_string(ip_address).to_ulong();
#endif
}

std::unique_ptr<boost_work> boost_make_work(boost_io_service& ioservice) {
#if BOOST_VERSION >= 106600
    return std::make_unique<boost_work>(boost::asio::make_work_guard(ioservice));
#else
    return std::make_unique<boost_work>(ioservice);
#endif
}


} // namespace restc_cpp
