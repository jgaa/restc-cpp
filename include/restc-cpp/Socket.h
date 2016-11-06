#pragma once
#ifndef RESTC_CPP_SOCKET_H_
#define RESTC_CPP_SOCKET_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif

#include <vector>

namespace restc_cpp {

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

    virtual void AsyncWrite(const write_buffers_t& buffers,
        boost::asio::yield_context& yield) = 0;

    virtual void AsyncConnect(const boost::asio::ip::tcp::endpoint& ep,
        boost::asio::yield_context& yield) = 0;

    virtual void AsyncShutdown(boost::asio::yield_context& yield) = 0;

    virtual void Close() = 0;

    virtual bool IsOpen() const noexcept = 0;

    friend std::ostream& operator << (std::ostream& o, const Socket& v) {
        return v.Print(o);
    }

protected:
    virtual std::ostream& Print(std::ostream& o) const = 0;
};


} // restc_cpp


#endif // RESTC_CPP_SOCKET_H_

