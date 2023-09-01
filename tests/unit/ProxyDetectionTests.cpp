// Include before boost::log headers
#include "restc-cpp/logging.h"

#include "restc-cpp/restc-cpp.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

#include <cstdlib>

using namespace std;
using namespace restc_cpp;

void clear_env() {
    unsetenv("https_proxy");
    unsetenv("HTTPS_PROXY");
    unsetenv("http_proxy");
    unsetenv("HTTP_PROXY");
}

TEST(ProxyDetection, ProxyTypeHTTP)
{
    clear_env();
    setenv("HTTP_PROXY", "http://1.2.3.4:8088", 1);

    Request::Properties properties;
    EXPECT_EQ(Request::Proxy::Type::HTTP, properties.proxy.detect());
    EXPECT_EQ(Request::Proxy::Type::HTTP, properties.proxy.type);
    EXPECT_EQ("http://1.2.3.4:8088", properties.proxy.address);

    setenv("http_proxy", "http://1.2.3.4:8089", 1);
    EXPECT_EQ(Request::Proxy::Type::HTTP, properties.proxy.detect());
    EXPECT_EQ(Request::Proxy::Type::HTTP, properties.proxy.type);
    EXPECT_EQ("http://1.2.3.4:8089", properties.proxy.address);
}

TEST(ProxyDetection, ProxyTypeHTTPS)
{
    clear_env();
    setenv("HTTP_PROXY", "http://1.2.3.4:8088", 1);
    setenv("HTTPS_PROXY", "http://1.2.3.4:8090", 1);

    Request::Properties properties;
    EXPECT_EQ(Request::Proxy::Type::HTTPS, properties.proxy.detect());
    EXPECT_EQ(Request::Proxy::Type::HTTPS, properties.proxy.type);
    EXPECT_EQ("http://1.2.3.4:8090", properties.proxy.address);

    setenv("https_proxy", "http://1.2.3.4:8091", 1);
    EXPECT_EQ(Request::Proxy::Type::HTTPS, properties.proxy.detect());
    EXPECT_EQ(Request::Proxy::Type::HTTPS, properties.proxy.type);
    EXPECT_EQ("http://1.2.3.4:8091", properties.proxy.address);
}

TEST(ProxyDetection, ProxyTypeNONE)
{
    clear_env();

    Request::Properties properties;
    EXPECT_EQ(Request::Proxy::Type::NONE, properties.proxy.detect());
    EXPECT_EQ(Request::Proxy::Type::NONE, properties.proxy.type);
    EXPECT_EQ("", properties.proxy.address);
}


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
