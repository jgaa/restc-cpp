
#include <iostream>

#include "restc-cpp/logging.h"

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"


using namespace std;
using namespace restc_cpp;


// For entries received from http://jsonplaceholder.typicode.com/posts
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

const string http_url = "http://localhost:3001/normal/manyposts";
const string https_url = "https://lastviking.eu/files/api";

#ifdef __unix__
#   include <cxxabi.h>
#endif

TEST(Gtest, validateOk) {
    EXPECT_EQ(1, 1);
}

TEST(Future, GetData) {
    auto client = RestClient::Create();
    EXPECT_TRUE(client);

    Post my_post;

    EXPECT_NO_THROW(
        my_post = client->ProcessWithPromiseT<Post>([&](Context& ctx) {
            Post post;
            SerializeFromJson(post, ctx.Get(GetDockerUrl(http_url) + "/1"));
            return post;
            }).get();
        ); // EXPECT_NO_THROW
    EXPECT_EQ(my_post.id, 1);
    EXPECT_FALSE(my_post.username.empty());
    EXPECT_FALSE(my_post.motto.empty());
}

// This test fails randomly. Could be a timing issue.
TEST(ExampleWorkflow, DISABLED_SequentialRequests) {
    auto cb = [](Context& ctx) -> void {
        // Asynchronously fetch the entire data-set, and convert it from json
        // to C++ objects was we go.
        // We expcet a list of Post objects
        list<Post> posts_list;
        SerializeFromJson(posts_list, ctx.Get(GetDockerUrl(http_url)));

        EXPECT_GE(posts_list.size(), 1);

        // Asynchronously connect to server and POST data.
        auto repl = ctx.Post(GetDockerUrl(http_url), "{\"test\":\"teste\"}");

        // Asynchronously fetch the entire data-set and return it as a string.
        auto json = repl->GetBodyAsString();
        RESTC_CPP_LOG_INFO_("Received POST data: " << json);
        EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);

        // Use RequestBuilder to fetch everything
        repl = RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url))
            .Header("X-Client", "RESTC_CPP")
            .Header("X-Client-Purpose", "Testing")
            .Header("Accept", "*/*")
            .Execute();

        string body = repl->GetBodyAsString();
        cout << "Got compressed list: " << body << endl;

        EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
        EXPECT_FALSE(body.empty());

        repl.reset();

        // Use RequestBuilder to fetch a record
        repl = RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url))
            .Header("X-Client", "RESTC_CPP")
            .Header("X-Client-Purpose", "Testing")
            .Header("Accept", "*/*")
            .Argument("id", 1)
            .Argument("test some $ stuff", "oh my my")
            .Execute();

        EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
        EXPECT_FALSE(body.empty());
        cout << "Got: " << repl->GetBodyAsString() << endl;
        repl.reset();

        // Use RequestBuilder to fetch a record without compression
        repl = RequestBuilder(ctx)
            .Get(GetDockerUrl(http_url))
            .Header("X-Client", "RESTC_CPP")
            .Header("X-Client-Purpose", "Testing")
            .Header("Accept", "*/*")
            .DisableCompression()
            .Argument("id", 2)
            .Execute();

        cout << "Got: " << repl->GetBodyAsString() << endl;
        repl.reset();

        // Use RequestBuilder to post a record
        Post data_object;
        data_object.username = "testid";
        data_object.motto = "Carpe diem";
        repl = RequestBuilder(ctx)
            .Post(GetDockerUrl(http_url))
            .Header("X-Client", "RESTC_CPP")
            .Data(data_object)
            .Execute();

        EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
        repl.reset();

#ifdef RESTC_CPP_WITH_TLS
            // Try with https
            repl = ctx.Get(https_url);
            json = repl->GetBodyAsString();
            EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
            EXPECT_FALSE(body.empty());
            RESTC_CPP_LOG_INFO_("Received https GET data: " << json);
#endif // TLS
            RESTC_CPP_LOG_INFO_("Done");
    };

    auto rest_client = RestClient::Create();
    auto future = rest_client->ProcessWithPromise(cb);

    // Hold the main thread to allow the worker to do it's job
    EXPECT_NO_THROW(future.get());
}

TEST(Request, HttpGetOk) {
    auto client = RestClient::Create();
    EXPECT_TRUE(client);

    client->Process([](Context& ctx) {
        try {
            auto repl = ctx.Get(GetDockerUrl(http_url));
            EXPECT_TRUE(repl);
            if (repl) {
                auto body = repl->GetBodyAsString();
                EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
                EXPECT_FALSE(body.empty());
            }
        } catch (const exception& ex) {
            RESTC_CPP_LOG_ERROR_("Request.HttpGetOk Caught exception: " << ex.what());
            EXPECT_FALSE(true);
        } catch (const boost::exception& ex) {
            RESTC_CPP_LOG_ERROR_("Request.HttpGetOk Caught boost exception: "
                << boost::diagnostic_information(ex));
            ostringstream estr;
#ifdef __unix__
            estr << " of type : " << __cxxabiv1::__cxa_current_exception_type()->name();
#endif
            RESTC_CPP_LOG_ERROR_("Request.HttpGetOk Caught unexpected exception " << estr.str());
            EXPECT_FALSE(true);
        }
        RESTC_CPP_IN_COROUTINE_CATCH_ALL(
            ostringstream estr;
#ifdef __unix__
            estr << " of type : " << __cxxabiv1::__cxa_current_exception_type()->name();
#endif
            RESTC_CPP_LOG_ERROR_("Request.HttpGetOk Caught unexpected exception " << estr.str());
            EXPECT_FALSE(true);
        ) // catch all
    });
}

TEST(RequestWithFuture, HttpGetOk) {
    auto client = RestClient::Create();
    EXPECT_TRUE(client);

    auto res = client->ProcessWithPromise([](Context& ctx) {
        auto repl = ctx.Get(GetDockerUrl(http_url));
        EXPECT_TRUE(repl);
        if (repl) {
            auto body = repl->GetBodyAsString();
            EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
            EXPECT_FALSE(body.empty());
        }
    });

    EXPECT_NO_THROW(res.get());
}

TEST(RequestBuilder, HttpGetOk) {
    auto client = RestClient::Create();
    EXPECT_TRUE(client);

    client->Process([](Context& ctx) {
        try {
        auto repl = RequestBuilder(ctx)
                    .Get(GetDockerUrl(http_url))
                    .Header("X-Client", "RESTC_CPP")
                    .Header("X-Client-Purpose", "Testing")
                    .Header("Accept", "*/*")
                    .Execute();

                const auto body = repl->GetBodyAsString();

                EXPECT_HTTP_OK(repl->GetHttpResponse().status_code);
                EXPECT_FALSE(body.empty());

        } catch (const exception& ex) {
            RESTC_CPP_LOG_ERROR_("RequestBuilder.HttpGetOk Caught exception: " << ex.what());
            EXPECT_FALSE(true);
        } catch (const boost::exception& ex) {
            RESTC_CPP_LOG_ERROR_("RequestBuilder.HttpGetOk Caught boost exception: "
                << boost::diagnostic_information(ex));
        } RESTC_CPP_IN_COROUTINE_CATCH_ALL(
            ostringstream estr;
#ifdef __unix__
            estr << " of type : " << __cxxabiv1::__cxa_current_exception_type()->name();
#endif
            RESTC_CPP_LOG_ERROR_("RequestBuilder.HttpGetOk Caught unexpected exception " << estr.str());
            EXPECT_FALSE(true);
        ) // catch all
    });
}


int main(int argc, char *argv[]) {

    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
