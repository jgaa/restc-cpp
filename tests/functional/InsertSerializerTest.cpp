
// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"


using namespace std;
using namespace restc_cpp;

struct Post {
    Post() = default;
    Post(string u, string m)
    : username{u}, motto{m} {}

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

TEST(TestInserter)
{
    Post post{"catch22", "Carpe Diem!"};
    CHECK_EQUAL(0, post.id);


    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

    auto reply = RequestBuilder(ctx)
        .Post(GetDockerUrl("http://localhost:3000/posts")) // URL
        .Data(post)                                 // Data object to send
        .Execute();                                 // Do it!

        CHECK_EQUAL(201, reply->GetResponseCode());

    }).get();
},

TEST(TestFunctorWriter)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        std::vector<Post> posts;
        posts.emplace_back("catch22", "Carpe Diem!");
        posts.emplace_back("catch23", "Carpe Diem! again!");
        posts.emplace_back("The answer is 42", "Really?");

        auto reply = RequestBuilder(ctx)
            .Post(GetDockerUrl("http://localhost:3001/upload_raw/")) // URL
            .DataProvider([&](DataWriter& writer) {

                RapidJsonInserter<Post> inserter(writer, true);

                for(const auto& post : posts) {
                    inserter.Add(post);
                }

                inserter.Done();

            })
            .Execute();

            CHECK_EQUAL(200, reply->GetResponseCode());

    }).get();

},

TEST(TestManualWriter)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        std::vector<Post> posts;
        posts.emplace_back("catch22", "Carpe Diem!");
        posts.emplace_back("catch23", "Carpe Diem! again!");
        posts.emplace_back("The answer is 42", "Really?");

        auto request = RequestBuilder(ctx)
            .Post(GetDockerUrl("http://localhost:3001/upload_raw/")) // URL
            .Chunked()
            .Build();


        {
            auto& writer = request->SendRequest(ctx);
            RapidJsonInserter<Post> inserter(writer, true);
            for(const auto& post : posts) {
                inserter.Add(post);
            }
        }

        auto reply = request->GetReply(ctx);
        CHECK_EQUAL(200, reply->GetResponseCode());

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
