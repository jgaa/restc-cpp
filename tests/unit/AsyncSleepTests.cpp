
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/error.h"
#include "restc-cpp/RequestBuilder.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"


using namespace std;
using namespace restc_cpp;

using namespace std::literals::chrono_literals;

const lest::test specification[] = {

TEST(TestSleepMilliseconds)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto start = std::chrono::steady_clock::now();
        ctx.Sleep(200ms);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - start).count();
        CHECK_CLOSE<int64_t>(200, duration, 50);

    }).get();
},

TEST(TestSleepSeconds)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto start = std::chrono::steady_clock::now();
        ctx.Sleep(1s);
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
                        (std::chrono::steady_clock::now() - start).count();
        CHECK_CLOSE<int64_t>(1000, duration, 50);

    }).get();
}


}; // lest


int main( int argc, char * argv[] )
{
#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );

#endif

    return lest::run( specification, argc, argv );
}
