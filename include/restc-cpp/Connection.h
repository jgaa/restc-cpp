#pragma once
#ifndef RESTC_CPP_CONNECTION_H_
#define RESTC_CPP_CONNECTION_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif


namespace restc_cpp {

class Socket;

class Connection {
public:
    using ptr_t = std::shared_ptr<Connection>;
    using release_callback_t = std::function<void (Connection&)>;

    enum class Type {
        HTTP,
        HTTPS
    };

    virtual ~Connection() = default;

    virtual Socket& GetSocket() = 0;
};

} // restc_cpp


#endif // RESTC_CPP_CONNECTION_H_

