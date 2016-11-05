#pragma once
#ifndef RESTC_CPP_CONNECTION_POOL_H_
#define RESTC_CPP_CONNECTION_POOL_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif


namespace restc_cpp {

class ConnectionPool {
public:
    virtual ~ConnectionPool() = default;
    
    virtual Connection::ptr_t GetConnection(
        const boost::asio::ip::tcp::endpoint ep,
        const Connection::Type connectionType,
        bool new_connection_please = false) = 0;

    virtual std::future<std::size_t> GetIdleConnections() const = 0;
    static std::unique_ptr<ConnectionPool> Create(RestClient& owner);
};

} // restc_cpp


#endif // RESTC_CPP_CONNECTION_POOL_H_

