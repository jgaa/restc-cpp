#include <assert.h>
#include <array>

#include <boost/utility/string_ref.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"
#include "restc-cpp/error.h"


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
        throw ParseException("Invalid protocol in url. Must be 'http[s]://'");
    }

    auto remains = boost::string_ref(protocol_name_.end());
    const auto args_start = remains.find('?');
    if (args_start != remains.npos) {
        args_ = {remains.begin() +  args_start + 1,
            remains.size() - (args_start + 1)};
        remains = {remains.begin(), args_start};
    }
    const auto port_start = remains.find(':');
    if (port_start != remains.npos) {
        if (remains.length() <= static_cast<decltype(host_.length())>(port_start + 2)) {
            throw ParseException("Invalid host (no port after column)");
        }
        //port_ = boost::string_ref(&remains[port_start+1]);
        //host_ = boost::string_ref(host_.data(), port_start);
        host_ = {remains.begin(), port_start};
        remains = {remains.begin() + port_start + 1, remains.size() - (port_start + 1)};

        const auto path_start = remains.find('/');
        if (path_start != port_.npos) {
            path_ = {remains.begin() + path_start, remains.size() - path_start};// &port_[path_start];
            port_ = {remains.begin(), path_start};
            remains = {};
        } else {
            port_ = remains;
        }
    } else {
        const auto path_start = remains.find('/');
        if (path_start != host_.npos) {
            //path_ = &host_[path_start];
            //host_ = boost::string_ref(host_.data(), path_start);

            host_ = {remains.begin(), path_start};
            path_ = {remains.begin() + host_.size(), remains.size() - host_.size()};
            remains = {};
        } else {
            host_ = remains;
        }
    }

    if (port_.empty()) {
        if (protocol_ == Protocol::HTTPS) {
            port_ = {"443"};
        } else {
            port_ = {"80"};
        }
    }

    return *this;
}

} // restc_cpp

