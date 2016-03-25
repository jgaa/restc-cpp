#pragma once
#ifndef RESTC_CPP_SOCKET_H_
#define RESTC_CPP_SOCKET_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif


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

    virtual void AsyncConnect(const boost::asio::ip::tcp::endpoint& ep,
    boost::asio::yield_context& yield) = 0;

    virtual void AsyncShutdown(boost::asio::yield_context& yield) = 0;
};


} // restc_cpp


#endif // RESTC_CPP_SOCKET_H_

