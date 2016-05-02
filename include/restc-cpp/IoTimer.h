#pragma once

#ifndef RESTC_CPP_IO_TIMER_H_
#define RESTC_CPP_IO_TIMER_H_

#include <iostream>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/Connection.h"

namespace restc_cpp {

class IoTimer : public std::enable_shared_from_this<IoTimer>
{
public:
    using ptr_t = std::shared_ptr<IoTimer>;
    using close_t = std::function<void ()>;

    ~IoTimer() {
        if (is_active_) {
            is_active_ = false;
        }
    }

    void Handler(const boost::system::error_code& error) {
        if (is_active_) {
            is_active_ = false;
            is_expiered_ = true;
            close_();
        }
    }

    void Cancel() {
        is_active_ = false;
    }

    bool IsExcpiered() const noexcept { return is_expiered_; }

    static ptr_t Create(int milliseconds_timeout,
        boost::asio::io_service& io_service,
        close_t close) {

        ptr_t timer;
        timer.reset(new IoTimer(io_service, close));
        timer->Start(milliseconds_timeout);
        return timer;
    }

    static ptr_t Create(int milliseconds_timeout,
        boost::asio::io_service& io_service,
            const Connection::ptr_t& connection) {

        if (!connection) {
            return nullptr;
        }

        std::weak_ptr<Connection> weak_connection = connection;

        return Create(milliseconds_timeout, io_service,
            [weak_connection]() {
                if (auto connection = weak_connection.lock()) {
                    if (connection->GetSocket().GetSocket().is_open()) {
                        RESTC_CPP_LOG_WARN << "Socket timed out.";
                        connection->GetSocket().GetSocket().close();
                    }
                }
            });
    }


private:
    IoTimer(boost::asio::io_service& io_service,
            close_t close)
    : close_{close}, timer_{io_service}
    {}

    void Start(int millisecondsTimeOut)
    {
        timer_.expires_from_now(
            boost::posix_time::milliseconds(millisecondsTimeOut));
        is_active_ = true;
        try {
            timer_.async_wait(std::bind(
                &IoTimer::Handler,
                shared_from_this(),
                std::placeholders::_1));
        } catch (const std::exception&) {
            is_active_ = false;
        }
    }

private:
    bool is_active_ = false;
    bool is_expiered_ = false;
    close_t close_;
    boost::asio::deadline_timer timer_;
};

} // restc_cpp

#endif // RESTC_CPP_IO_TIMER_H_

