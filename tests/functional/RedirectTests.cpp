

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"

#include "../src/ReplyImpl.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_redirect_url = "http://localhost:3001/redirect/posts";
const string http_reredirect_url = "http://localhost:3001/reredirect/posts";
const string http_redirect_loop_url = "http://localhost:3001/loop/posts";

using namespace std;
using namespace restc_cpp;

TEST(Redirect, NoRedirects)
{
    Request::Properties properties;
    properties.maxRedirects = 0;

    auto rest_client = RestClient::Create(properties);
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        EXPECT_THROW(
            ctx.Get(GetDockerUrl(http_redirect_url)), ConstraintException);

    });

    EXPECT_NO_THROW(f.get());
}

TEST(Redirect, SingleRedirect)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl = ctx.Get(GetDockerUrl(http_redirect_url));
        EXPECT_EQ(200, repl->GetResponseCode());
        // Discard all data
        while(repl->MoreDataToRead()) {
            repl->GetSomeData();
        }

    });

    EXPECT_NO_THROW(f.get());
}

TEST(Redirect, DoubleRedirect)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl = ctx.Get(GetDockerUrl(http_reredirect_url));
        EXPECT_EQ(200, repl->GetResponseCode());
        // Discard all data
        while(repl->MoreDataToRead()) {
            repl->GetSomeData();
        }

    });

    EXPECT_NO_THROW(f.get());
}

TEST(Redirect, RedirectLoop)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        EXPECT_THROW(
            ctx.Get(GetDockerUrl(http_redirect_loop_url)), ConstraintException);

    });

    EXPECT_NO_THROW(f.get());
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}

