
#include <iostream>
#include <boost/lexical_cast.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Serialize.h"

using namespace std;
using namespace restc_cpp;


// For entries received from http://jsonplaceholder.typicode.com/posts
struct Post {
    int user_id = 0;
    int id = 0;
    string title;
    string body;
};


const string http_url = "http://jsonplaceholder.typicode.com/posts";
const string https_url = "https://jsonplaceholder.typicode.com/posts";


void DoSomethingInteresting(Context& ctx) {

    Serialize<Post> post_serializer = {
        DECL_FIELD_JN(Post, int, userId, user_id),
        DECL_FIELD(Post, int, id),
        DECL_FIELD(Post, std::string, title),
        DECL_FIELD(Post, std::string, body)
    };

    try {
        // We expcet a list of Post objects
        std::list<Post> posts_list;

        // Create a root handler for our list of objects
        auto json_handler = CreateRootRapidJsonHandler<
            RapidJsonHandlerObjectArray<Post>>(posts_list, post_serializer);

        // Asynchronously fetch the entire data-set, and convert it from json
        // to C++ objects was we go.
        json_handler->FetchAll(ctx.Get(http_url));

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
