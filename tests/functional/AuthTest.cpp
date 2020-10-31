
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

const lest::test specification[] = {

TEST(TestFailedAuth)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        EXPECT_THROWS_AS(ctx.Get(GetDockerUrl("http://localhost:3001/restricted/posts/1")),
                    HttpAuthenticationException);

    }).get();
},

TEST(TestSuccessfulAuth)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Get(GetDockerUrl("http://localhost:3001/restricted/posts/1"))
            .BasicAuthentication("alice", "12345")
            .Execute();

        CHECK_EQUAL(200, reply->GetResponseCode());

    }).get();
}

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
