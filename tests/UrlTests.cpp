

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"

#include "UnitTest++/UnitTest++.h"

using namespace std;
using namespace restc_cpp;

TEST(UrlSimple)
{
    Url url("http://github.com");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("80", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(UrlSimpleSlash)
{
    Url url("http://github.com/");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("80", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(UrlWithPath)
{
    Url url("http://github.com/jgaa");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("80", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa", url.GetPath());
}

TEST(UrlWithPathAndSlash)
{
    Url url("http://github.com/jgaa/");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("80", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa/", url.GetPath());
}

TEST(HttpWithPort)
{
    Url url("http://github.com:56");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("56", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpWithLongPort)
{
    Url url("http://github.com:1234567789");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("1234567789", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpWithPortAndSlash)
{
    Url url("http://github.com:56/");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("56", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpWithPortAndPath)
{
    Url url("http://github.com:12345/jgaa");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("12345", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa", url.GetPath());
}

TEST(HttpWithPortAndPathPath)
{
    Url url("http://github.com:12345/jgaa/andmore");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("12345", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTP, url.GetProtocol());
    CHECK_EQUAL("/jgaa/andmore", url.GetPath());
}

TEST(UrlSimpleHttps)
{
    Url url("https://github.com");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("443", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

/////
TEST(HttpsUrlSimpleSlash)
{
    Url url("https://github.com/");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("443", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpsUrlWithPath)
{
    Url url("https://github.com/jgaa");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("443", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa", url.GetPath());
}

TEST(HttpsUrlWithPathAndSlash)
{
    Url url("https://github.com/jgaa/");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("443", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa/", url.GetPath());
}

TEST(HttpsWithPort)
{
    Url url("https://github.com:56");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("56", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpsWithLongPort)
{
    Url url("https://github.com:1234567789");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("1234567789", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpsWithPortAndSlash)
{
    Url url("https://github.com:56/");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("56", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}

TEST(HttpsWithPortAndPath)
{
    Url url("https://github.com:12345/jgaa");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("12345", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa", url.GetPath());
}

TEST(HttpsWithPortAndPathPath)
{
    Url url("https://github.com:12345/jgaa/andmore");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("12345", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/jgaa/andmore", url.GetPath());
}

TEST(HttpsUrlSimple)
{
    Url url("https://github.com");
    CHECK_EQUAL("github.com", url.GetHost());
    CHECK_EQUAL("443", url.GetPort());
    CHECK_EQUAL(Url::Protocol::HTTPS, url.GetProtocol());
    CHECK_EQUAL("/", url.GetPath());
}


int main(int, const char *[])
{
   return UnitTest::RunAllTests();
}

