// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/fusion/adapted.hpp>


#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;

TEST(Url, Simple)
{
    Url url("http://github.com");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("80"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, UrlSimpleSlash)
{
    Url url("http://github.com/");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("80"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, UrlWithPath)
{
    Url url("http://github.com/jgaa");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("80"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/jgaa"s, url.GetPath());
}

TEST(Url, UrlWithPathAndSlash)
{
    Url url("http://github.com/jgaa/");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("80"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/jgaa/"s, url.GetPath());
}

TEST(Url, UrlWithPathInclColon)
{
    Url url("http://github.com/jgaa:test");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("80"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/jgaa:test"s, url.GetPath());
}

TEST(Url, HttpWithPort)
{
    Url url("http://github.com:56");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("56"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpWithLongPort)
{
    Url url("http://github.com:1234567789");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("1234567789"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpWithPortAndSlash)
{
    Url url("http://github.com:56/");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("56"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpWithPortAndPath)
{
    Url url("http://github.com:12345/jgaa");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("12345"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/jgaa"s, url.GetPath());
}

TEST(Url, HttpWithPortAndPathInclColon)
{
    Url url("http://github.com:12345/jgaa:test");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("12345"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/jgaa:test"s, url.GetPath());
}

TEST(Url, HttpWithPortAndPathPath)
{
    Url url("http://github.com:12345/jgaa/andmore");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("12345"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTP, url.GetProtocol());
    EXPECT_EQ("/jgaa/andmore"s, url.GetPath());
}

TEST(Url, UrlSimpleHttps)
{
    Url url("https://github.com");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("443"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

/////
TEST(Url, HttpsUrlSimpleSlash)
{
    Url url("https://github.com/");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("443"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpsUrlWithPath)
{
    Url url("https://github.com/jgaa");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("443"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/jgaa"s, url.GetPath());
}

TEST(Url, HttpsUrlWithPathAndSlash)
{
    Url url("https://github.com/jgaa/");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("443"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/jgaa/"s, url.GetPath());
}

TEST(Url, HttpsWithPort)
{
    Url url("https://github.com:56");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("56"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpsWithLongPort)
{
    Url url("https://github.com:1234567789");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("1234567789"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpsWithPortAndSlash)
{
    Url url("https://github.com:56/");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("56"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
}

TEST(Url, HttpsWithPortAndPath)
{
    Url url("https://github.com:12345/jgaa");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("12345"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/jgaa"s, url.GetPath());
}

TEST(Url, HttpsWithPortAndPathPath)
{
    Url url("https://github.com:12345/jgaa/andmore");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("12345"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/jgaa/andmore"s, url.GetPath());
}

TEST(Url, HttpsUrlSimple)
{
    Url url("https://github.com");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("443"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
    EXPECT_EQ(""s, url.GetArgs());
}


TEST(Url, HttpsWithPortAndPathAndArgs)
{
    Url url("https://github.com:12345/jgaa?arg=abc:5432");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("12345"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/jgaa"s, url.GetPath());
    EXPECT_EQ("arg=abc:5432"s, url.GetArgs());
}

TEST(Url, HttpsWithArgsOnly)
{
    Url url("https://github.com?arg=abc:123");
    EXPECT_EQ("github.com"s, url.GetHost());
    EXPECT_EQ("443"s, url.GetPort());
    EXPECT_EQ_ENUM(Url::Protocol::HTTPS, url.GetProtocol());
    EXPECT_EQ("/"s, url.GetPath());
    EXPECT_EQ("arg=abc:123"s, url.GetArgs());
    const string args{"arg=abc:123"};
    EXPECT_EQ(args, url.GetArgs());
    EXPECT_EQ(args.size(), url.GetArgs().size());
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
