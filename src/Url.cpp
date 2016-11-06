#include <assert.h>
#include <array>

#include <boost/utility/string_ref.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"


using namespace std;

std::ostream& operator <<(std::ostream& out,
                          const restc_cpp::Url::Protocol& protocol) {
    static const array<string, 3> names = {{"UNKNOWN", "HTTP", "HTTPS"}};

    return out << names.at(static_cast<unsigned>(protocol));
}

namespace restc_cpp {

Url::Url(const char *url)
{
    operator = (url);
}

Url& Url::operator = (const char *url) {
    assert(url != nullptr && "A valid URL is required");
    protocol_name_ = boost::string_ref(url);
    if (protocol_name_.find("https://") == 0) {
        protocol_name_ = boost::string_ref(url, 8);
        protocol_ = Protocol::HTTPS;
    } else if (protocol_name_.find("http://") == 0) {
        protocol_name_ = boost::string_ref(url, 7);
        protocol_ = Protocol::HTTP;
    } else {
        throw invalid_argument("Invalid protocol in url. Must be 'http[s]://'");
    }

    host_ = boost::string_ref(protocol_name_.end());
    const auto port_start = host_.find(':');
    if (port_start != host_.npos) {
        if (host_.length() <= static_cast<decltype(host_.length())>(port_start + 2)) {
            throw invalid_argument("Invalid host (no port after column)");
        }
        port_ = boost::string_ref(&host_[port_start+1]);
        host_ = boost::string_ref(host_.data(), port_start);

        const auto path_start = port_.find('/');
        if (path_start != port_.npos) {
            path_ = &port_[path_start];
            port_ = boost::string_ref(port_.data(), path_start);
        }
    } else {
        const auto path_start = host_.find('/');
        if (path_start != host_.npos) {
            path_ = &host_[path_start];
            host_ = boost::string_ref(host_.data(), path_start);
        }
    }

    if (port_.empty()) {
        if (protocol_ == Protocol::HTTPS) {
            port_ = {"443"};
        } else {
            port_ = {"80"};
        }
    }
}

} // restc_cpp

