
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"

using namespace std;
using namespace restc_cpp;


// For entries received from http://jsonplaceholder.typicode.com/posts
struct Post {
    int userId = 0;
    int id = 0;
    string title;
    string body;
};

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, userId)
    (int, id)
    (string, title)
    (string, body)
)

const string http_url = "http://jsonplaceholder.typicode.com/posts";
const string https_url = "https://jsonplaceholder.typicode.com/posts";


void DoSomethingInteresting(Context& ctx) {


    try {
        // Asynchronously fetch the entire data-set, and convert it from json
        // to C++ objects was we go.
        // We expcet a list of Post objects
        list<Post> posts_list;
        SerializeFromJson(posts_list, ctx.Get(http_url));

        // Just dump the data.
        for(const auto& post : posts_list) {
            RESTC_CPP_LOG_INFO << "Post id=" << post.id << ", title: " << post.title;
        }

        // Asynchronously connect to server and POST data.
        auto repl = ctx.Post(http_url, "{ 'test' : 'teste' }");

        // Asynchronously fetch the entire data-set and return it as a string.
        auto json = repl->GetBodyAsString();
        RESTC_CPP_LOG_INFO << "Received POST data: " << json;

#ifdef RESTC_CPP_WITH_TLS
        // Try with https
        repl = ctx.Get(https_url);
        json = repl->GetBodyAsString();
        RESTC_CPP_LOG_INFO << "Received https GET data: " << json;
#endif // TLS
        RESTC_CPP_LOG_INFO << "Done";

    } catch (const exception& ex) {
        RESTC_CPP_LOG_INFO << "Process: Caught exception: " << ex.what();
    }
}


int main(int argc, char *argv[]) {

    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );

    try {
        auto rest_client = RestClient::Create();
        auto future = rest_client->ProcessWithPromise(DoSomethingInteresting);

        // Hold the main thread to allow the worker to do it's job
        future.wait();
    } catch (const exception& ex) {
        RESTC_CPP_LOG_INFO << "main: Caught exception: " << ex.what();
    }

    return 0;
}
