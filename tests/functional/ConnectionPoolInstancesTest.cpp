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
const string http_url = "http://localhost:3001/normal/posts";
const string http_url_many = "http://localhost:3001/normal/manyposts";
const string http_connection_close_url = "http://localhost:3001/close/posts";

using namespace std;
using namespace restc_cpp;


TEST(ConnectionPoolInstances, UseAfterDelete) {

    for(auto i = 0; i < 500; ++i) {

        RestClient::Create()->ProcessWithPromiseT<int>([&](Context& ctx) {
            auto repl = ctx.Get(GetDockerUrl(http_url));
            EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
            EXPECT_NO_THROW(repl->GetBodyAsString());
            return 0;
        }).get();

        RestClient::Create()->ProcessWithPromiseT<int>([&](Context& ctx) {
            auto repl = ctx.Get(GetDockerUrl(http_url));
            EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
            EXPECT_NO_THROW(repl->GetBodyAsString());
            return 0;
        }).get();

        if ((i % 100) == 0) {
            clog << '#' << (i +1) << endl;
        }
    }
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
