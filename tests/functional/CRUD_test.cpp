

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

using namespace std;
using namespace restc_cpp;


/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_url = "http://localhost:3000/posts";

struct Post {
    int id = 0;
    string username;
    string motto;
};

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, id)
    (string, username)
    (string, motto)
)


const lest::test specification[] = {

STARTCASE(TestCRUD) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

    Post post;
    post.username = "catch22";
    post.motto = "Carpe Diem!";

    CHECK_EQUAL(0, post.id);

    auto reply = RequestBuilder(ctx)
        .Post(GetDockerUrl(http_url)) // URL
        .Data(post)                                 // Data object to send
        .Execute();                                 // Do it!


    // The mock server returns the new record.
    Post svr_post;
    SerializeFromJson(svr_post, *reply);

    CHECK_EQUAL(post.username, svr_post.username);
    CHECK_EQUAL(post.motto, svr_post.motto);
    EXPECT(svr_post.id > 0);

    // Change the data
    post = svr_post;
    post.motto = "Change!";
    reply = RequestBuilder(ctx)
        .Put(GetDockerUrl(http_url) + "/" + to_string(post.id)) // URL
        .Data(post)                                 // Data object to update
        .Execute();

    // Fetch again
    reply = RequestBuilder(ctx)
        .Get(GetDockerUrl(http_url) + "/" + to_string(post.id)) // URL
        .Execute();
    SerializeFromJson(svr_post, *reply);
    CHECK_EQUAL(post.motto, svr_post.motto);

    // Delete
    reply = RequestBuilder(ctx)
        .Delete(GetDockerUrl(http_url) + "/" + to_string(post.id)) // URL
        .Execute();

    // Verify that it's gone
    EXPECT_THROWS_AS(
        RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url) + "/" + to_string(post.id)) // URL
            .Execute(), RequestFailedWithErrorException);



    }).get();

} ENDCASE

STARTCASE(TestOptions) {

    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Options(GetDockerUrl(http_url)) // URL
            .Execute();  // Do it!

        CHECK_EQUAL(204, reply->GetResponseCode());

    }).get();

} ENDCASE

STARTCASE(TestHEAD) {

    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Head(GetDockerUrl(http_url)) // URL
            .Execute();  // Do it!

        CHECK_EQUAL(200, reply->GetResponseCode());

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
