
#include <bitset>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/url_encode.h"

using namespace std;

namespace restc_cpp {

namespace {

constexpr size_t bitset_size = 255;
using allchars_t = std::bitset<bitset_size>;

allchars_t get_normal_ch() {
    allchars_t bits;
    for(size_t chv = 0; chv < bitset_size; ++chv) {
        const auto ch = static_cast<uint8_t>(chv);
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
    constexpr auto magic_4 = 4;
    constexpr auto magic_0x0f = 0x0f;

    static const string hex{"0123456789ABCDEF"};
    static auto normal_ch = get_normal_ch();
    std::string rval;
    rval.reserve(src.size() * 2);

    for(auto ch : src) {
        if (normal_ch[static_cast<uint8_t>(ch)]) {
            rval += ch;
        } else {
            rval += '%';
            rval += hex[(ch >> magic_4) & magic_0x0f];
            rval += hex[ch & magic_0x0f];
        }
    }
    return rval;
}

} // namespace
