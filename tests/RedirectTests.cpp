

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "../src/ReplyImpl.h"

#include "UnitTest++/UnitTest++.h"

#include "restc_cpp_testing.h"

/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_redirect_url = "http://localhost:3001/redirect/posts";
const string http_reredirect_url = "http://localhost:3001/reredirect/posts";
const string http_redirect_loop_url = "http://localhost:3001/loop/posts";

using namespace std;
using namespace restc_cpp;

namespace restc_cpp{
namespace unittests {


TEST(TestNoRedirects)
{
    Request::Properties properties;
    properties.maxRedirects = 0;

    auto rest_client = RestClient::Create(properties);
    rest_client->ProcessWithPromise([&](Context& ctx) {

        CHECK_THROW(
            ctx.Get(GetDockerUrl(http_redirect_url)), ConstraintException);

    }).get();
}

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
}


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
}

TEST(TestRedirectLoop)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        CHECK_THROW(
            ctx.Get(GetDockerUrl(http_redirect_loop_url)), ConstraintException);

    }).get();
}


}} // namespaces

int main(int, const char *[])
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );

    return UnitTest::RunAllTests();
}

