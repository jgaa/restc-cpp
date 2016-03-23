#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"

using namespace std;

namespace restc_cpp {

    class ConnectionImpl : public Connection {
    public:

        ConnectionImpl(std::unique_ptr<Socket> socket)
        : socket_{std::move(socket)}
        {
        }

    Socket& GetSocket() override {
        return *socket_;
    }

    private:
        std::unique_ptr<Socket> socket_;
    };

} // restc_cpp

