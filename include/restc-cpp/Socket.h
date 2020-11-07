#pragma once
#ifndef RESTC_CPP_SOCKET_H_
#define RESTC_CPP_SOCKET_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif

#include <vector>

#include <boost/system/error_code.hpp>

#include "restc-cpp/typename.h"
#include "error.h"

namespace restc_cpp {

class Socket
{
public:
    enum class Reason {
      DONE,
      TIME_OUT
    };

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
		const std::string &host,
        bool tcpNodelay,
        boost::asio::yield_context& yield) = 0;

    virtual void AsyncShutdown(boost::asio::yield_context& yield) = 0;

    virtual void Close(Reason reoson = Reason::DONE) = 0;

    virtual bool IsOpen() const noexcept = 0;

    friend std::ostream& operator << (std::ostream& o, const Socket& v) {
        return v.Print(o);
    }

protected:
    virtual std::ostream& Print(std::ostream& o) const = 0;
};


class ExceptionWrapper {
protected:
    template <typename Tret, typename Tfn>
    Tret WrapException(Tfn fn) {
        try {
            return fn();
        } catch (const boost::system::system_error& ex) {
            if (ex.code().value() == boost::system::errc::operation_canceled) {
                if (reason_ == Socket::Reason::TIME_OUT) {
                    throw RequestTimeOutException();
                }
            }
            throw;
        }
    }

    boost::optional<Socket::Reason> reason_;
};

} // restc_cpp


#endif // RESTC_CPP_SOCKET_H_

