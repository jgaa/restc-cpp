// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/fusion/adapted.hpp>


#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

using namespace std;
using namespace restc_cpp;

const lest::test specification[] = {

STARTCASE(UrlSimple)
{
    Url url("http://github.com");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(UrlSimpleSlash)
{
    Url url("http://github.com/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(UrlWithPath)
{
    Url url("http://github.com/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
} ENDCASE

STARTCASE(UrlWithPathAndSlash)
{
    Url url("http://github.com/jgaa/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("80"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpWithPort)
{
    Url url("http://github.com:56");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpWithLongPort)
{
    Url url("http://github.com:1234567789");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("1234567789"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpWithPortAndSlash)
{
    Url url("http://github.com:56/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpWithPortAndPath)
{
    Url url("http://github.com:12345/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
} ENDCASE

STARTCASE(HttpWithPortAndPathPath)
{
    Url url("http://github.com:12345/jgaa/andmore");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa/andmore"s, url.GetPath());
} ENDCASE

STARTCASE(UrlSimpleHttps)
{
    Url url("https://github.com");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

/////
STARTCASE(HttpsUrlSimpleSlash)
{
    Url url("https://github.com/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsUrlWithPath)
{
    Url url("https://github.com/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsUrlWithPathAndSlash)
{
    Url url("https://github.com/jgaa/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsWithPort)
{
    Url url("https://github.com:56");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsWithLongPort)
{
    Url url("https://github.com:1234567789");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("1234567789"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsWithPortAndSlash)
{
    Url url("https://github.com:56/");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("56"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsWithPortAndPath)
{
    Url url("https://github.com:12345/jgaa");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsWithPortAndPathPath)
{
    Url url("https://github.com:12345/jgaa/andmore");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa/andmore"s, url.GetPath());
} ENDCASE

STARTCASE(HttpsUrlSimple)
{
    Url url("https://github.com");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
    CHECK_EQUAL(""s, url.GetArgs());
} ENDCASE


STARTCASE(HttpsWithPortAndPathAndArgs)
{
    Url url("https://github.com:12345/jgaa?arg=abc:5432");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("12345"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa"s, url.GetPath());
    CHECK_EQUAL("arg=abc:5432"s, url.GetArgs());
} ENDCASE

STARTCASE(HttpsWithArgsOnly)
{
    Url url("https://github.com?arg=abc:123");
    CHECK_EQUAL("github.com"s, url.GetHost());
    CHECK_EQUAL("443"s, url.GetPort());
    CHECK_EQUAL_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/"s, url.GetPath());
    CHECK_EQUAL("arg=abc:123"s, url.GetArgs());
    const string args{"arg=abc:123"};
    CHECK_EQUAL(args, url.GetArgs());
    CHECK_EQUAL(args.size(), url.GetArgs().size());
} ENDCASE


}; // lest


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    return lest::run( specification, argc, argv );
}
