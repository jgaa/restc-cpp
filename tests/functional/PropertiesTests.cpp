

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

using namespace std;
using namespace restc_cpp;


/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_url = "http://localhost:3000/fail";


const lest::test specification[] = {

STARTCASE(Test404) {

    Request::Properties properties;
    properties.throwOnHttpError = false;

    auto rest_client = RestClient::Create(properties);
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url)) // URL
            .Execute();  // Do it!

        CHECK_EQUAL(404, reply->GetResponseCode());

    }).get();

} ENDCASE


}; //lest

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    return lest::run( specification, argc, argv );
}
