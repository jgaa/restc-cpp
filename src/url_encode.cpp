
#include <bitset>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/url_encode.h"

using namespace std;

namespace restc_cpp {

namespace {
std::bitset<255> get_normal_ch() {
    std::bitset<255> bits;

    for(uint8_t ch = 0; ch < bits.size(); ++ch) {
        if ((ch >= '0' && ch <= '9')
            || (ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z')
            || ch == '-' || ch == '_' || ch == '.'
            || ch == '!' || ch == '~' || ch == '*'
            || ch == '\'' || ch == '(' || ch == ')'
            || ch == '/')

        {
            bits[ch] = true;
        }
    }

    return bits;
}

} // anonymous namespace

std::string url_encode(const boost::string_ref& src) {

    static const string hex{"0123456789ABCDEF"};
    static auto normal_ch = get_normal_ch();
    std::string rval;
    rval.reserve(src.size());


    for(auto ch : src) {
        if (normal_ch[static_cast<uint8_t>(ch)]) {
            rval += ch;
        } else {
            rval += '%';
            rval += hex[(ch >> 4) & 0x0f];
            rval += hex[ch & 0x0f];
        }
    }
    return rval;
}

} // namespace
