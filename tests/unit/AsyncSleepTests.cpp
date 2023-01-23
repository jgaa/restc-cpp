
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

using namespace std::literals::chrono_literals;

TEST(AsyncSleep, SleepMilliseconds)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        auto start = std::chrono::steady_clock::now();
        ctx.Sleep(200ms);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - start).count();
        EXPECT_CLOSE(200, duration, 50);

    });

    EXPECT_NO_THROW(f.get());
}

TEST(AsyncSleep, TestSleepSeconds)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        auto start = std::chrono::steady_clock::now();
        ctx.Sleep(1s);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - start).count();
        EXPECT_CLOSE(1000, duration, 50);

    });

    EXPECT_NO_THROW(f.get());
}


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
