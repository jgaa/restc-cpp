

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;


/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_url = "http://localhost:3000/fail";

TEST(Properties, Res404) {

    Request::Properties properties;
    properties.throwOnHttpError = false;

    auto rest_client = RestClient::Create(properties);
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url)) // URL
            .Execute();  // Do it!

        EXPECT_EQ(404, reply->GetResponseCode());

    });

    EXPECT_NO_THROW(f.get());
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
