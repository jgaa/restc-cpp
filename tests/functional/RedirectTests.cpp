

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

#include "../src/ReplyImpl.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"


/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_redirect_url = "http://localhost:3001/redirect/posts";
const string http_reredirect_url = "http://localhost:3001/reredirect/posts";
const string http_redirect_loop_url = "http://localhost:3001/loop/posts";

using namespace std;
using namespace restc_cpp;

const lest::test specification[] = {

TEST(TestNoRedirects)
{
    Request::Properties properties;
    properties.maxRedirects = 0;

    auto rest_client = RestClient::Create(properties);
    rest_client->ProcessWithPromise([&](Context& ctx) {

        EXPECT_THROWS_AS(
            ctx.Get(GetDockerUrl(http_redirect_url)), ConstraintException);

    }).get();
},

TEST(TestSingleRedirect)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl = ctx.Get(GetDockerUrl(http_redirect_url));
        CHECK_EQUAL(200, repl->GetResponseCode());
        // Discard all data
        while(repl->MoreDataToRead()) {
            repl->GetSomeData();
        }

    }).get();
},

TEST(TestDoubleRedirect)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl = ctx.Get(GetDockerUrl(http_reredirect_url));
        CHECK_EQUAL(200, repl->GetResponseCode());
        // Discard all data
        while(repl->MoreDataToRead()) {
            repl->GetSomeData();
        }

    }).get();
},

TEST(TestRedirectLoop)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        EXPECT_THROWS_AS(
            ctx.Get(GetDockerUrl(http_redirect_loop_url)), ConstraintException);

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

