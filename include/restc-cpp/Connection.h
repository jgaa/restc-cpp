#pragma once
#ifndef RESTC_CPP_CONNECTION_H_
#define RESTC_CPP_CONNECTION_H_

#ifndef RESTC_CPP_H_
#   error "Include restc-cpp.h first"
#endif

namespace restc_cpp {

class Socket;

class Connection {
public:
    using ptr_t = std::shared_ptr<Connection>;

    enum class Type {
        HTTP,
        HTTPS
    };

    virtual ~Connection() = default;

    virtual boost::uuids::uuid GetId() const = 0;
    virtual Socket& GetSocket() = 0;
    virtual const Socket& GetSocket() const = 0;

    friend std::ostream& operator << (std::ostream& o, const Connection& v) {
        return v.Print(o);
    }

private:
    std::ostream& Print(std::ostream& o) const;
};

} // restc_cpp


#endif // RESTC_CPP_CONNECTION_H_
