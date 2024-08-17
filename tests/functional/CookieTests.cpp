#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;


static const string address = GetDockerUrl("http://localhost:3001/cookies/");

TEST(Cookies, HaveCookies)
{
    auto rest_client = RestClient::Create();

    rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get(address)
            .Execute();

        auto cookies = reply->GetHeaders("Set-Cookie");
        EXPECT_EQ(3, cookies.size());

        set<string> allowed = {"test1=yes", "test2=maybe", "test3=no"};

        for (const auto &c : cookies) {
            EXPECT_EQ(true, allowed.find(c) != allowed.end());
            allowed.erase(c);
        }

        EXPECT_EQ(0, allowed.size());

    }).get();
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
