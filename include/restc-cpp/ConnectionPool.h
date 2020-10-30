
#pragma once
#ifndef RESTC_CPP_CONNECTION_POOL_H_
#define RESTC_CPP_CONNECTION_POOL_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif


namespace restc_cpp {

class ConnectionPool
{
public:
    using ptr_t = std::shared_ptr<ConnectionPool>;
    virtual ~ConnectionPool() = default;

    virtual Connection::ptr_t GetConnection(
        const boost::asio::ip::tcp::endpoint ep,
        const Connection::Type connectionType,
        bool new_connection_please = false) = 0;

    virtual std::future<std::size_t> GetIdleConnections() const = 0;
    static std::shared_ptr<ConnectionPool> Create(RestClient& owner);

    /*! Close the connection-pool
     *
     * This is an internal method.
     *
     * This method is not thread-safe. It must be run by the
     * RestClients worker-thread.
     */
    virtual void Close() = 0;
};

} // restc_cpp


#endif // RESTC_CPP_CONNECTION_POOL_H_

