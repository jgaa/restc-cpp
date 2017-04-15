

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

using namespace std;
using namespace restc_cpp;

const lest::test specification[] = {

TEST(UrlSimple)
{
    Url url("http://github.com");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(UrlSimpleSlash)
{
    Url url("http://github.com/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(UrlWithPath)
{
    Url url("http://github.com/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
},

TEST(UrlWithPathAndSlash)
{
    Url url("http://github.com/jgaa/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa/"s, url.GetPath());
},

TEST(HttpWithPort)
{
    Url url("http://github.com:56");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpWithLongPort)
{
    Url url("http://github.com:1234567789");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("1234567789"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpWithPortAndSlash)
{
    Url url("http://github.com:56/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpWithPortAndPath)
{
    Url url("http://github.com:12345/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
},

TEST(HttpWithPortAndPathPath)
{
    Url url("http://github.com:12345/jgaa/andmore");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa/andmore"s, url.GetPath());
},

TEST(UrlSimpleHttps)
{
    Url url("https://github.com");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

/////
TEST(HttpsUrlSimpleSlash)
{
    Url url("https://github.com/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpsUrlWithPath)
{
    Url url("https://github.com/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
},

TEST(HttpsUrlWithPathAndSlash)
{
    Url url("https://github.com/jgaa/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa/"s, url.GetPath());
},

TEST(HttpsWithPort)
{
    Url url("https://github.com:56");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpsWithLongPort)
{
    Url url("https://github.com:1234567789");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("1234567789"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpsWithPortAndSlash)
{
    Url url("https://github.com:56/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
},

TEST(HttpsWithPortAndPath)
{
    Url url("https://github.com:12345/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
},

TEST(HttpsWithPortAndPathPath)
{
    Url url("https://github.com:12345/jgaa/andmore");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa/andmore"s, url.GetPath());
},

TEST(HttpsUrlSimple)
{
    Url url("https://github.com");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
}
}; // lest


int main( int argc, char * argv[] )
{
    return lest::run( specification, argc, argv );
}
