#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

using namespace std;
using namespace restc_cpp;


static const string defunct_proxy_address = GetDockerUrl("http://172.17.0.1:0");
static const string http_proxy_address = GetDockerUrl("http://172.17.0.1:3003");
static const string socks5_proxy_address = GetDockerUrl("172.17.0.1:3004");

const lest::test specification[] = {

STARTCASE(TestFailToConnect)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTP;
    properties.proxy.address = defunct_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);


    EXPECT_THROWS(rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();

    }).get());
} ENDCASE

STARTCASE(TestWithHttpProxy)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTP;
    properties.proxy.address = http_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();

            cout << "Got: " << reply->GetBodyAsString() << endl;
    }).get();
} ENDCASE

STARTCASE(TestWithSocks5Proxy)
{
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::SOCKS5;
    properties.proxy.address = socks5_proxy_address;

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);

    rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get("http://api.example.com/normal/posts/1")
            .Execute();

            cout << "Got: " << reply->GetBodyAsString() << endl;
    }).get();
} ENDCASE

}; //lest


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");

    return lest::run( specification, argc, argv );
}
