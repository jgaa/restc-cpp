#pragma once

#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/logging.h"

namespace restc_cpp {

class SocketImpl : public Socket, protected ExceptionWrapper {
public:

    SocketImpl(boost::asio::io_service& io_service)
    : socket_{io_service}
    {
    }

    boost::asio::ip::tcp::socket& GetSocket() override {
        return socket_;
    }

    const boost::asio::ip::tcp::socket& GetSocket() const override {
        return socket_;
    }

    std::size_t AsyncReadSome(boost::asio::mutable_buffers_1 buffers,
                            boost::asio::yield_context& yield) override {
        return WrapException<std::size_t>([&] {
            return socket_.async_read_some(buffers, yield);
        });
    }

    std::size_t AsyncRead(boost::asio::mutable_buffers_1 buffers,
                        boost::asio::yield_context& yield) override {
        return WrapException<std::size_t>([&] {
            return boost::asio::async_read(socket_, buffers, yield);
        });
    }

    void AsyncWrite(const boost::asio::const_buffers_1& buffers,
                    boost::asio::yield_context& yield) override {
        boost::asio::async_write(socket_, buffers, yield);
    }

    void AsyncWrite(const write_buffers_t& buffers,
                    boost::asio::yield_context& yield)override {

        return WrapException<void>([&] {
            boost::asio::async_write(socket_, buffers, yield);
        });
    }

    void AsyncConnect(const boost::asio::ip::tcp::endpoint& ep,
					const std::string &host,
                    bool tcpNodelay,
                    boost::asio::yield_context& yield) override {
        return WrapException<void>([&] {
            socket_.async_connect(ep, yield);
            socket_.lowest_layer().set_option(boost::asio::ip::tcp::no_delay(tcpNodelay));
            OnAfterConnect();
        });
    }

    void AsyncShutdown(boost::asio::yield_context& yield) override {
        // Do nothing.
    }

    void Close(Reason reason) override {
        if (socket_.is_open()) {
            RESTC_CPP_LOG_TRACE_("Closing " << *this);
            socket_.close();
        }
        reason_ = reason;
    }

    bool IsOpen() const noexcept override {
        return socket_.is_open();
    }

protected:
    std::ostream& Print(std::ostream& o) const override {
        if (IsOpen()) {
            const auto& socket = GetSocket();
            o << "{Socket "
                << "socket# "
                << static_cast<int>(
                const_cast<boost::asio::ip::tcp::socket&>(socket).native_handle());
            try {
                return o << " " << socket.local_endpoint()
                    << " <--> " << socket.remote_endpoint() << '}';
            } catch (const std::exception& ex) {
                o << " {std exception: " << ex.what() << "}}";
            }
        }

        return o << "{Socket (unused/closed)}";
    }

private:
    boost::asio::ip::tcp::socket socket_;
};


} // restc_cpp

