

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


TEST(CRUD, Crud) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

    Post post;
    post.username = "catch22";
    post.motto = "Carpe Diem!";

    EXPECT_EQ(0, post.id);

    auto reply = RequestBuilder(ctx)
        .Post(GetDockerUrl(http_url)) // URL
        .Data(post)                                 // Data object to send
        .Execute();                                 // Do it!


    // The mock server returns the new record.
    Post svr_post;
    SerializeFromJson(svr_post, *reply);

    EXPECT_EQ(post.username, svr_post.username);
    EXPECT_EQ(post.motto, svr_post.motto);
    EXPECT_TRUE(svr_post.id > 0);

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
    EXPECT_EQ(post.motto, svr_post.motto);

    // Delete
    reply = RequestBuilder(ctx)
        .Delete(GetDockerUrl(http_url) + "/" + to_string(post.id)) // URL
        .Execute();

    // Verify that it's gone
    EXPECT_THROW(
        RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url) + "/" + to_string(post.id)) // URL
            .Execute(), HttpNotFoundException);



    }).get();
}

TEST(CRUD, Options) {

    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Options(GetDockerUrl(http_url)) // URL
            .Execute();  // Do it!

        EXPECT_EQ(204, reply->GetResponseCode());

    }).get();

}

TEST(CRUD, HEAD) {

    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Head(GetDockerUrl(http_url)) // URL
            .Execute();  // Do it!

        EXPECT_EQ(200, reply->GetResponseCode());

    }).get();

}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
