
// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

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

TEST(InsertSerializer, Inserter)
{
    Post post{"catch22", "Carpe Diem!"};
    EXPECT_EQ(0, post.id);


    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

    auto reply = RequestBuilder(ctx)
        .Post(GetDockerUrl("http://localhost:3000/posts")) // URL
        .Data(post)                                 // Data object to send
        .Execute();                                 // Do it!

        EXPECT_HTTP_OK(reply->GetResponseCode());

    });

    EXPECT_NO_THROW(f.get());
}

TEST(InsertSerializer, FunctorWriter)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

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

            EXPECT_EQ(200, reply->GetResponseCode());

    });

    EXPECT_NO_THROW(f.get());
}

TEST(InsertSerializer, ManualWriter)
{
    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context& ctx) {

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
        EXPECT_HTTP_OK(reply->GetResponseCode());

    });

    EXPECT_NO_THROW(f.get());
}


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
