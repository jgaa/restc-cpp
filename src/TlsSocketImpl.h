#pragma once

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"
#ifdef RESTC_CPP_WITH_TLS

#include <boost/asio/ssl.hpp>
#include "restc-cpp/Socket.h"

namespace restc_cpp {

class TlsSocketImpl : public Socket {
public:

    using ssl_socket_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    TlsSocketImpl(boost::asio::io_service& io_service)
    {
        tls_context_.set_options(boost::asio::ssl::context::default_workarounds
            | boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::no_sslv3
            | boost::asio::ssl::context::no_tlsv1_1
            | boost::asio::ssl::context::single_dh_use);

        ssl_socket_ = std::make_unique<ssl_socket_t>(io_service, tls_context_);
    }

    boost::asio::ip::tcp::socket& GetSocket() override {
        return static_cast<boost::asio::ip::tcp::socket&>(
            ssl_socket_->lowest_layer());
    }

    const boost::asio::ip::tcp::socket& GetSocket() const override {
        return  static_cast<const boost::asio::ip::tcp::socket&>(
            ssl_socket_->lowest_layer());
    }

    std::size_t AsyncReadSome(boost::asio::mutable_buffers_1 buffers,
                              boost::asio::yield_context& yield) override {
        return ssl_socket_->async_read_some(buffers, yield);
    }

    std::size_t AsyncRead(boost::asio::mutable_buffers_1 buffers,
                          boost::asio::yield_context& yield) override {
        return boost::asio::async_read(*ssl_socket_, buffers, yield);
    }

    void AsyncWrite(const boost::asio::const_buffers_1& buffers,
                    boost::asio::yield_context& yield) override {
        boost::asio::async_write(*ssl_socket_, buffers, yield);
    }

    void AsyncWrite(const write_buffers_t& buffers,
                    boost::asio::yield_context& yield) override {
        boost::asio::async_write(*ssl_socket_, buffers, yield);
    }

    void AsyncConnect(const boost::asio::ip::tcp::endpoint& ep,
                      boost::asio::yield_context& yield) override {
        GetSocket().async_connect(ep, yield);
        ssl_socket_->async_handshake(boost::asio::ssl::stream_base::client,
                                     yield);
    }

    void AsyncShutdown(boost::asio::yield_context& yield) override {
        ssl_socket_->async_shutdown(yield);
    }

    void Close() override {
        if (ssl_socket_->lowest_layer().is_open()) {
            RESTC_CPP_LOG_TRACE << "Closing " << *this;
            ssl_socket_->lowest_layer().close();
        }
    }

    bool IsOpen() const noexcept override {
        return ssl_socket_->lowest_layer().is_open();
    }

protected:
    std::ostream& Print(std::ostream& o) const override {
        if (IsOpen()) {
            const auto& socket = GetSocket();
            return o << "{TlsSocket "
                << "socket# "
                << static_cast<int>(
                const_cast<boost::asio::ip::tcp::socket&>(socket).native_handle())
                << " " << socket.local_endpoint()
                << " <--> " << socket.remote_endpoint() << '}';
        }

        return o << "{TlsSocket (unused/closed)}";
    }


private:
    boost::asio::ssl::context tls_context_{boost::asio::ssl::context::sslv23 };
    std::unique_ptr<ssl_socket_t> ssl_socket_;
};


} // restc_cpp

#endif // RESTC_CPP_WITH_TLS

