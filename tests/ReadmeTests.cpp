#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

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

// The C++ main function - the place where any adventure starts
void first() {

    // Create and instantiate a Post from data received from the server.
    Post my_post = RestClient::Create()->ProcessWithPromiseT<Post>([&](Context& ctx) {
        // This is a co-routine, running in a worker-thread

        // Instantiate a Post structure.
        Post post;

        // Serialize it asynchronously. The asynchronously part does not really matter
        // here, but it may if you receive huge data structures.
        SerializeFromJson(post,

            // Construct a request to the server
            RequestBuilder(ctx)
                .Get("http://jsonplaceholder.typicode.com/posts/1")

                // Add some headers for good taste
                .Header("X-Client", "RESTC_CPP")
                .Header("X-Client-Purpose", "Testing")

                // Send the request
                .Execute());

        // Return the post instance trough a C++ future<>
        return post;
    })

    // Get the Post instance from the future<>, or any C++ exception thrown
    // within the lambda.
    .get();

    // Print the result for everyone to see.
    cout << "Received post# " << my_post.id << ", title: " << my_post.title;
}


void DoSomethingInteresting(Context& ctx) {
    // Here we are again in a co-routine, running in a worker-thread.

    // Asynchronously connect to a server and fetch some data.
    auto reply = ctx.Get("http://jsonplaceholder.typicode.com/posts/1");

    // Asynchronously fetch the entire data-set and return it as a string.
    auto json = reply->GetBodyAsString();

    // Just dump the data.
    cout << "Received data: " << json << endl;
}

void second() {
    auto rest_client = RestClient::Create();

    // Call DoSomethingInteresting as a co-routine in a worker-thread.
    rest_client->Process(DoSomethingInteresting);

    // Wait for a little while to allow the worker-thread to finish
    this_thread::sleep_for(chrono::seconds(5));
}

void third() {

    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {
        // Here we are again in a co-routine, running in a worker-thread.

        // Asynchronously connect to a server and fetch some data.
        auto reply = RequestBuilder(ctx)
            .Get("http://localhost:3001/restricted/posts/1")

            // Authenticate as 'alice' with a very popular password
            .BasicAuthentication("alice", "12345")

            // Send the request.
            .Execute();

        // Dump the well protected data
        cout << "Got: " << reply->GetBodyAsString();

    }).get();
}

void forth() {

    // Add the proxy information to the properties used by the client
    Request::Properties properties;
    properties.proxy.type = Request::Proxy::Type::HTTP;
    properties.proxy.address = "http://127.0.0.1:3003";

    // Create the client with our configuration
    auto rest_client = RestClient::Create(properties);
    rest_client->ProcessWithPromise([&](Context& ctx) {
        // Here we are again in a co-routine, running in a worker-thread.

        // Asynchronously connect to a server trough a HTTP proxy and fetch some data.
        auto reply = RequestBuilder(ctx)
            .Get("http://fwd/normal/posts/1")

            // Send the request.
            .Execute();

        // Dump the data
        cout << "Got: " << reply->GetBodyAsString();

    }).get();
}



int main() {
    try {
        cout << "First: " << endl;
        first();

        cout << "Second: " << endl;
        second();

        cout << "Third: " << endl;
        third();

        cout << "Forth: " << endl;
        forth();

    } catch(const exception& ex) {
        cerr << "Something threw up: " << ex.what() << endl;
    }
}
