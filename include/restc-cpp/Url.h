#pragma once

#ifndef RESTC_CPP_URL_H_
#define RESTC_CPP_URL_H_

#include <boost/utility/string_view.hpp>

namespace restc_cpp {


 class Url {
 public:
     enum class Protocol {
         UNKNOWN,
         HTTP,
         HTTPS
     };

     Url(const char *url);

     Url& operator = (const char *url);

     boost::string_view GetProtocolName() const { return protocol_name_; }
     boost::string_view GetHost() const { return host_; }
     boost::string_view GetPort() const { return port_; }
     boost::string_view GetPath() const { return path_; }
     boost::string_view GetArgs() const { return args_; }
     Protocol GetProtocol() const { return protocol_; }

 private:
     boost::string_view protocol_name_;
     boost::string_view host_;
     boost::string_view port_;
     boost::string_view path_ = "/";
     boost::string_view args_;
     Protocol protocol_ = Protocol::UNKNOWN;
 };

} // namespace restc_cpp

std::ostream& operator <<(std::ostream& out,
                          const restc_cpp::Url::Protocol& protocol);

#endif // RESTC_CPP_URL_H_
