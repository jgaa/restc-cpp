

// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "UnitTest++/UnitTest++.h"

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


namespace restc_cpp{
namespace unittests {


TEST(TestCRUD)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

    Post post;
    post.username = "catch22";
    post.motto = "Carpe Diem!";

    CHECK_EQUAL(0, post.id);

    auto reply = RequestBuilder(ctx)
        .Post(http_url) // URL
        .Data(post)                                 // Data object to send
        .Execute();                                 // Do it!


    // The mock server returns the new record.
    Post svr_post;
    SerializeFromJson(svr_post, *reply);

    CHECK_EQUAL(post.username, svr_post.username);
    CHECK_EQUAL(post.motto, svr_post.motto);
    CHECK(svr_post.id > 0);

    // Change the data
    post = svr_post;
    post.motto = "Change!";
    reply = RequestBuilder(ctx)
        .Put(http_url + "/" + to_string(post.id)) // URL
        .Data(post)                                 // Data object to update
        .Execute();

    // Fetch again
    reply = RequestBuilder(ctx)
        .Get(http_url + "/" + to_string(post.id)) // URL
        .Execute();
    SerializeFromJson(svr_post, *reply);
    CHECK_EQUAL(post.motto, svr_post.motto);

    // Delete
    reply = RequestBuilder(ctx)
        .Delete(http_url + "/" + to_string(post.id)) // URL
        .Execute();

    // Verify that it's gone
    CHECK_THROW(
        RequestBuilder(ctx)
            .Get(http_url + "/" + to_string(post.id)) // URL
            .Execute(), RequestFailedWithErrorException);



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

