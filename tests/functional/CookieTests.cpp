#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

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
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );
    return lest::run( specification, argc, argv );
}
