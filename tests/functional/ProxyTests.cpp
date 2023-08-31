#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;


static const string defunct_proxy_address = GetDockerUrl("http://localhost:3777");
static const string http_proxy_address = GetDockerUrl("http://localhost:3003");
static const string https_proxy_address = GetDockerUrl("http://localhost:3003");
static const string socks5_proxy_address = GetDockerUrl("localhost:3004");

TEST(Proxy, FailToConnect_HTTP_Proxy)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTP;
    properties.proxy.address = defunct_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);


    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();
    });

    //EXPECT_ANY_THROW(f.get());
    EXPECT_THROW(f.get(), FailedToConnectException);
}

TEST(Proxy, With_HTTP_Proxy)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTP;
    properties.proxy.address = http_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();

        EXPECT_HTTP_OK(reply->GetResponseCode());
        cout << "Got: " << reply->GetBodyAsString() << endl;
    });

    EXPECT_NO_THROW(f.get());
}


TEST(Proxy, FailToConnect_HTTPSCONNECT_Proxy)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTPS;
    properties.proxy.address = defunct_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);


    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("https://api.example.com/normal/posts/1")
            .Execute();
    });

    EXPECT_THROW(f.get(), FailedToConnectException);
}

TEST(Proxy, With_HTTPSCONNECT_ProxyToHttps)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTPS;
    properties.proxy.address = https_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("https://api.example.com/normal/posts/1")
            .Execute();

        EXPECT_HTTP_OK(reply->GetResponseCode());
        cout << "Got: " << reply->GetBodyAsString() << endl;
    });

    EXPECT_NO_THROW(f.get());
}

// WARNING: passing plain http over a CONNECT tunnel is denied by default in squid
// we enabled it in squid.conf for the sake of our test
TEST(Proxy, With_HTTPSCONNECT_ProxyToHttp)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTPS;
    properties.proxy.address = https_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();

        EXPECT_HTTP_OK(reply->GetResponseCode());
        cout << "Got: " << reply->GetBodyAsString() << endl;
    });

    EXPECT_NO_THROW(f.get());
}

// we denied CONNECT method to port 444 in our squid.conf
TEST(Proxy, With_HTTPSCONNECT_ProxyToDeniedPort)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTPS;
    properties.proxy.address = https_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("https://api.example.com:444/normal/posts/1")
            .Execute();
    });

    EXPECT_THROW(f.get(), FailedToConnectException);
}


TEST(Proxy, With_SOCKS5_Proxy)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::SOCKS5;
    properties.proxy.address = socks5_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();

        EXPECT_HTTP_OK(reply->GetResponseCode());
        cout << "Got: " << reply->GetBodyAsString() << endl;
    });

    EXPECT_NO_THROW(f.get());
}


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
