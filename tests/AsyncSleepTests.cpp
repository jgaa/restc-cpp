
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/error.h"
#include "restc-cpp/RequestBuilder.h"

#include "UnitTest++/UnitTest++.h"

using namespace std;
using namespace restc_cpp;

using namespace std::literals::chrono_literals;

TEST(TestSleepMilliseconds)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto start = std::chrono::steady_clock::now();
        ctx.Sleep(200ms);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - start).count();
        CHECK_CLOSE(200, duration, 50);

    }).get();
}

TEST(TestSleepSeconds)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto start = std::chrono::steady_clock::now();
        ctx.Sleep(1s);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - start).count();
        CHECK_CLOSE(1000, duration, 50);

    }).get();
}


int main(int, const char *[])
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );

    return UnitTest::RunAllTests();
}

