#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "ConnectionImpl.h"
#include "SocketImpl.h"
#include "TlsSocketImpl.h"


using namespace std;

namespace restc_cpp {

class ConnectionWrapper : public Connection
{
public:
    ConnectionWrapper(shared_ptr<Connection> connection,
                      release_callback_t on_release = nullptr)
    : on_release_{on_release}, connection_{connection}
    {
        if (on_release_) {
            on_release_(*this);
        }
    }

    Socket& GetSocket() override {
        return connection_->GetSocket();
    }

    ~ConnectionWrapper() {
    }

private:
    release_callback_t on_release_;
    shared_ptr<Connection> connection_;
};

class ConnectionPoolImpl : public ConnectionPool {
public:

    ConnectionPoolImpl(RestClient& owner)
    : owner_{owner}
    {
    }

Connection::ptr_t
GetConnection(const boost::asio::ip::tcp::endpoint ep,
              const Connection::Type connectionType,
              bool new_connection_please) override {

    // TODO: Implement the pool
                  
    unique_ptr<Socket> socket;
    if (connectionType == Connection::Type::HTTP) {
        socket = make_unique<SocketImpl>(owner_.GetIoService());
    } else {
        socket = make_unique<TlsSocketImpl>(owner_.GetIoService());
    }

    auto connection = make_unique<ConnectionWrapper>(
        make_shared<ConnectionImpl>(move(socket)));

    return move(connection);
}

private:
    RestClient& owner_;
};


std::unique_ptr<ConnectionPool>
ConnectionPool::Create(RestClient& owner) {
    return make_unique<ConnectionPoolImpl>(owner);
}


} // restc_cpp

