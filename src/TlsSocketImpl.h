#pragma once

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"

#include <boost/asio/ssl.hpp>
#include <boost/version.hpp>

#include "restc-cpp/Socket.h"
#include "restc-cpp/config.h"

#if !defined(RESTC_CPP_WITH_TLS)
#   error "Do not include when compiling without TLS"
#endif

namespace restc_cpp {

class TlsSocketImpl : public Socket, protected ExceptionWrapper {
public:

    using ssl_socket_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

    TlsSocketImpl(boost::asio::io_service& io_service,
            shared_ptr<boost::asio::ssl::context> ctx,
            bool can_send_over_unupgraded_socket = false)
    {
        ssl_socket_ = std::make_unique<ssl_socket_t>(io_service, *ctx);
        can_send_over_unupgraded_socket_ = can_send_over_unupgraded_socket;
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
        return WrapException<std::size_t>([&] {
            if (can_send_over_unupgraded_socket_)
                return ssl_socket_->next_layer().async_read_some(buffers, yield);
            return ssl_socket_->async_read_some(buffers, yield);
        });
    }

    std::size_t AsyncRead(boost::asio::mutable_buffers_1 buffers,
                          boost::asio::yield_context& yield) override {
        return WrapException<std::size_t>([&] {
            if (can_send_over_unupgraded_socket_)
                return boost::asio::async_read(ssl_socket_->next_layer(), buffers, yield);
            return boost::asio::async_read(*ssl_socket_, buffers, yield);
        });
    }

    void AsyncWrite(const boost::asio::const_buffers_1& buffers,
                    boost::asio::yield_context& yield) override {
        if (can_send_over_unupgraded_socket_)
            boost::asio::async_write(ssl_socket_->next_layer(), buffers, yield);
        else
            boost::asio::async_write(*ssl_socket_, buffers, yield);
    }

    void AsyncWrite(const write_buffers_t& buffers,
                    boost::asio::yield_context& yield) override {
        return WrapException<void>([&] {
            if (can_send_over_unupgraded_socket_)
                boost::asio::async_write(ssl_socket_->next_layer(), buffers, yield);
            else
                boost::asio::async_write(*ssl_socket_, buffers, yield);
        });
    }

    void AsyncConnect(const boost::asio::ip::tcp::endpoint& ep,
                    const string &host,
                    bool tcpNodelay,
                    boost::asio::yield_context& yield) override {
        return WrapException<void>([&] {
            //TLS-SNI (without this option, handshakes attempts with hosts behind CDNs will fail,
            //due to the fact that the CDN does not have enough information at the TLS layer
            //to decide where to forward the handshake attempt).

            RESTC_CPP_LOG_TRACE_("AsyncConnect - Calling SSL_set_tlsext_host_name --> " << host);
            SSL_set_tlsext_host_name(ssl_socket_->native_handle(), host.c_str());

            RESTC_CPP_LOG_TRACE_("AsyncConnect - Calling async_connect");
            GetSocket().async_connect(ep, yield);

            RESTC_CPP_LOG_TRACE_("AsyncConnect - Calling lowest_layer().set_option");
            ssl_socket_->lowest_layer().set_option(
                        boost::asio::ip::tcp::no_delay(tcpNodelay));

            RESTC_CPP_LOG_TRACE_("AsyncConnect - Calling OnAfterConnect()");
            OnAfterConnect();

            RESTC_CPP_LOG_TRACE_("AsyncConnect - Calling async_handshake");
            ssl_socket_->async_handshake(boost::asio::ssl::stream_base::client,
                                         yield);
            can_send_over_unupgraded_socket_ = false;

            RESTC_CPP_LOG_TRACE_("AsyncConnect - Done");
        });
    }

    void AsyncShutdown(boost::asio::yield_context& yield) override {
        return WrapException<void>([&] {
            ssl_socket_->async_shutdown(yield);
        });
    }

    void Close(Reason reason) override {
        if (ssl_socket_->lowest_layer().is_open()) {
            RESTC_CPP_LOG_TRACE_("Closing " << *this);
            ssl_socket_->lowest_layer().close();
        }
        reason_ = reason;
    }

    bool IsOpen() const noexcept override {
        return ssl_socket_->lowest_layer().is_open();
    }

protected:
    std::ostream& Print(std::ostream& o) const override {
        if (IsOpen()) {
            const auto& socket = GetSocket();
            o << "{TlsSocket "
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

        return o << "{TlsSocket (unused/closed)}";
    }


private:
    std::unique_ptr<ssl_socket_t> ssl_socket_;
    bool can_send_over_unupgraded_socket_;
};

} // restc_cpp


