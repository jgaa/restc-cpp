
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"

using namespace std;
using namespace restc_cpp;


// For entries received from http://jsonplaceholder.typicode.com/posts
struct Post {
    int user_id = 0;
    int id = 0;
    string title;
    string body;
};

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, user_id)
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
        for(auto post : posts_list) {
            cout << "Post id=" << post.id << ", title: " << post.title << endl;
        }

        // Asynchronously connect to server and POST data.
        auto repl = ctx.Post(http_url, "{ 'test' : 'teste' }");

        // Asynchronously fetch the entire data-set and return it as a string.
        auto json = repl->GetBodyAsString();
        clog << "Received POST data: " << json << endl;

        // Try with https
        repl = ctx.Get(https_url);
        json = repl->GetBodyAsString();
        clog << "Received https GET data: " << json << endl;

        clog << "Done" << endl;

    } catch (const exception& ex) {
        std::clog << "Process: Caught exception: " << ex.what() << endl;
    }
}


int main(int argc, char *argv[]) {

    try {
        auto rest_client = RestClient::Create();
        rest_client->Process(DoSomethingInteresting);

        // Hold the main thread to allow the worker to do it's job
        cin.get();
        // TODO: Shut down the client and wait for the worker thread to exit.
    } catch (const exception& ex) {
        std::clog << "main: Caught exception: " << ex.what() << endl;
    }

    return 0;
}
