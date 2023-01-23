#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;


// C++ structure that match the Json entries received
// from http://jsonplaceholder.typicode.com/posts/{id}
struct Post {
    int userId = 0;
    int id = 0;
    string title;
    string body;
};

// Add C++ reflection to the Post structure.
// This allows our Json (de)serialization to do it's magic.
BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, userId)
    (int, id)
    (string, title)
    (string, body)
)


// https://jsonplaceholder.typicode.com does not work with boost::asio
// with tls on Fedora 25. It seems to be a problem with incompatible
// cipher suites. I have to try with different versions of openssl/libressl
// to see if it is a Fedora thing or a more general problem.
// Anyway, I plan to refactor the tls code as a general DataReader
// to get better control and more flexibility about what crypto
// library to use (boost::asio only support openssl and compatible
// libraries out of the box).

//string https_url = "https://jsonplaceholder.typicode.com/posts/1";

string https_url = "https://lastviking.eu/files/api";

TEST(Https, GetJson) {
    shared_ptr<boost::asio::ssl::context> tls_ctx = make_shared<boost::asio::ssl::context>(boost::asio::ssl::context{ boost::asio::ssl::context::tlsv12_client});

    EXPECT_NO_THROW(tls_ctx->set_options(boost::asio::ssl::context::default_workarounds
                    | boost::asio::ssl::context::no_sslv2
                    | boost::asio::ssl::context::no_sslv3
//                        | boost::asio::ssl::context::no_tlsv1_1
                    | boost::asio::ssl::context::single_dh_use);
    );

    auto client = RestClient::Create(tls_ctx);
    auto f = client->ProcessWithPromise([&](Context& ctx) {
        Post post;
        auto reply = ctx.Get(https_url);
        EXPECT_HTTP_OK(reply->GetResponseCode());
        SerializeFromJson(post, *reply);
        EXPECT_EQ(1, post.id);
    });

    EXPECT_NO_THROW(f.get());
}

int main(int argc, char * argv[])
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
