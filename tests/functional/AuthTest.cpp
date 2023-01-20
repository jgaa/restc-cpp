
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/error.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"


using namespace std;
using namespace restc_cpp;

TEST(Auth, Failed) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        EXPECT_THROW(ctx.Get(GetDockerUrl("http://localhost:3001/restricted/posts/1")),
                    HttpAuthenticationException);

    }).get();
}

TEST(Auth, Success) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Get(GetDockerUrl("http://localhost:3001/restricted/posts/1"))
            .BasicAuthentication("alice", "12345")
            .Execute();

        EXPECT_EQ(200, reply->GetResponseCode());

    }).get();
}


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
