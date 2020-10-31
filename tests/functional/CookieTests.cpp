#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

using namespace std;
using namespace restc_cpp;


static const string address = GetDockerUrl("http://localhost:3001/cookies/");

const lest::test specification[] = {

STARTCASE(TestHaveCookies)
{
    auto rest_client = RestClient::Create();

    rest_client->ProcessWithPromise([&](Context& ctx) {
        auto reply = RequestBuilder(ctx)
            .Get(address)
            .Execute();

        auto cookies = reply->GetHeaders("Set-Cookie");
        CHECK_EQUAL(3, cookies.size());

        set<string> allowed = {"test1=yes", "test2=maybe", "test3=no"};

        for(const auto c : cookies) {
            CHECK_EQUAL(true, allowed.find(c) != allowed.end());
            allowed.erase(c);
        }

        CHECK_EQUAL(0, allowed.size());

    }).get();
} ENDCASE

}; //lest


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
