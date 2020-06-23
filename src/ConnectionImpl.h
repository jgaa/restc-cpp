#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>
#include <boost/uuid/uuid_generators.hpp> // generators

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {

std::ostream& Connection::Print(std::ostream& o) const {
    return o << "{Connection "
        << GetId()
        << ' '
        << GetSocket()
        << '}';
}


class ConnectionImpl : public Connection {
public:

    ConnectionImpl(std::unique_ptr<Socket> socket)
    : socket_{std::move(socket)}
    {
        RESTC_CPP_LOG_TRACE_(*this << " is constructed.");
    }

    ~ConnectionImpl() {
        RESTC_CPP_LOG_TRACE_(*this << " is dead.");
    }

    Socket& GetSocket() override {
        return *socket_;
    }

    const Socket& GetSocket() const override {
        return *socket_;
    }

    boost::uuids::uuid GetId() const override {
        return id_;
    }

private:
    std::unique_ptr<Socket> socket_;
    const boost::uuids::uuid id_ = boost::uuids::random_generator()();
};

} // restc_cpp

