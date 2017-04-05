
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

#include "restc_cpp_testing.h"

using namespace std;
using namespace restc_cpp;


TEST(TestFailedAuth)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        CHECK_THROW(ctx.Get(GetDockerUrl("http://localhost:3001/restricted/posts/1")),
                    HttpAuthenticationException);

    }).get();
}

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


int main(int, const char *[])
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );

    return UnitTest::RunAllTests();
}
