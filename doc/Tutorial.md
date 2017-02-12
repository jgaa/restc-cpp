# Tutiorial

Here are some more examples about common use-cases.

## Fetch json data list and convert to native C++ types.

Another example - here we fetch a json list like this one and convert
it to a std::list of a native C++ data type

```json
[
  {
    "userId": 1,
    "id": 1,
    "title": "sunt aut facere repellat provident occaecati excepturi optio reprehenderit",
    "body": "quia et suscipit\nsuscipit recusandae consequuntur expedita et cum\nreprehenderit molestiae ut ut quas totam\nnostrum rerum est autem sunt rem eveniet architecto"
  },
  {
    "userId": 1,
    "id": 2,
    "title": "qui est esse",
    "body": "est rerum tempore vitae\nsequi sint nihil reprehenderit dolor beatae ea dolores neque\nfugiat blanditiis voluptate porro vel nihil molestiae ut reiciendis\nqui aperiam non debitis possimus qui neque nisi nulla"
  },
...
```

```C++
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/RequestBuilder.h"

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

void DoSomethingInteresting(Context& ctx) {

     try {
        // Asynchronously fetch the entire data-set, and convert it from json
        // to C++ objects was we go.
        // We expcet a list of Post objects
        list<Post> posts_list;
        SerializeFromJson(posts_list,
            ctx.Get("http://jsonplaceholder.typicode.com/posts"));

        // Just dump the data.
        for(const auto& post : posts_list) {
            clog << "Post id=" << post.id << ", title: " << post.title << endl;
        }

    } catch (const exception& ex) {
        clog << "Caught exception: " << ex.what() << endl;
    }
}

```

In the example above, we fetch all the data into the <i>posts_list</i>. If you
receive really large lists, this may not be a good ide, as your RAM would fill
up and eventually your application would hang or die.

Another, much better approach, is to fetch the list into a C++ input iterator.
In this case, we only store one data item in memory at any time (well, two
if you use the <i>it++</i> operator - something you only do when you really
need the data before the increment - right?). The size of the list does not
matter. The HTTP client will fetch data asynchronously in the beckground
when it is parsing the json stream to instatiate a data object. When an
object is fetched from the stream, you can access it from the iterator.
Only when you access one of the increment methods on the iterator will the
library concern itself with the next data object.

```C++
int main() {
    // Fetch a list of records asyncrouesly, one by one.
    // This allows us to process single items in a list
    // and fetching more data as we move forward.
    // This works basically as a database cursor, or
    // (literally) as a properly implemented C++ input iterator.

    // Create the REST clent
    auto rest_client = RestClient::Create();

    // Run our example in a lambda co-routine
    rest_client->Process([&](Context& ctx) {
        // This is the co-routine, running in a worker-thread

        // Construct a request to the server
        auto reply = RequestBuilder(ctx)
            .Get("http://jsonplaceholder.typicode.com/posts/")

            // Add some headers for good taste
            .Header("X-Client", "RESTC_CPP")
            .Header("X-Client-Purpose", "Testing")

            // Send the request
            .Execute();

        // Instatiate a serializer with begin() and end() methods that
        // allows us to work with the reply-data trough a C++
        // input iterator.
        IteratorFromJsonSerializer<Post> data{*reply};

        // Iterate over the data, fetch data asyncrounesly as we go.
        for(const auto& post : data) {
            cout << "Item #" << post.id << " Title: " << post.title << endl;
        }
    });


    // Wait for the request to finish
    rest_client->CloseWhenReady(true);
}
```

You can use futures to synchronize the
requests, and to get exceptions from failed requests.
In the example below we use a lambda as our coroutine.

```C++
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

int main()
{
    // Set up logging
    namespace logging = boost::log;
    logging::core::get()->set_filter (
        logging::trivial::severity >= logging::trivial::debug
    );

    // Create a rest client.
    auto rest_client = RestClient::Create();

    // Do the work in a lambda
    auto done = rest_client->ProcessWithPromise([&](Context& ctx) {
        // Here we are executing the coroutine in a worker thread.
        // The worker thread belongs to the RestClient instance.
        // We can run a large number of concurrent, independent
        // coroutines with this thread.

        // Fetch some data
        auto repl = ctx.Get("https://example.com/api/data");

        // Do something
        ...

        // Exit the coroutine
    });

    try {
        // The calling thread waits here for the coroutine to finish.
        // (The worker thread is still available, as the RestClient
        // instance (rest_client) is still in scope.)
        done.get();
    } catch(const exception& ex) {
        clog << "Main thread: Caught exception from coroutine: "
            << ex.what() << endl;
    }
}

```

## Post data to the server
```C++

    // Use the RequestBuilder and POST a json serialized native C++ object
    SomeDataClass data_object;
    data_object.userid   = "catch22";
    data_object.motto    = "Carpe Diem!";

    auto reply = RequestBuilder(ctx)
        .Post("https://example.com/api/data")              // URL
        .Header("X-Client", "RESTC_CPP")                   // Optional header
        .Data(data_object)                                 // Data object to send
        .Execute();                                        // Do it!

```

## Upload a file to the server
```C++
    // Upload a file to a HTTP service
    reply = RequestBuilder(ctx)
        .Post("http://example.com/upload")                 // URL
        .Header("X-Client", "RESTC_CPP")                   // Optional header
        .Argument("filename", "cute.jpg")                  // Optional URL argument
        .File("/var/data/cats/cute.jpg")                   // The file to send
        .Execute();                                        // Do it!

    clog << "Done" << endl;
```

## Send a request using HTTP Basic Authentication
```C++

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

        // Dump the well guarded data
        cout << "Got: " << reply->GetBodyAsString();

    }).get();

```

## Send a request going trough a HTTP Proxy
```C++
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
            .Get("http://api.example.com/normal/posts/1")

            // Send the request.
            .Execute();

        // Dump the data
        cout << "Got: " << reply->GetBodyAsString();

    }).get();
```

## Use an existing thread in stead of a worker thread

This example is slightly more advanced. Here we take
responsibility to run the io-service used internally by
the rest client instance. This allow us to use an existing
thread, like the applications main thread, to handle the
asynchronous requests.

```C++
    Request::Properties properties;

    // Create the client without creating a worker thread
    auto rest_client = RestClient::Create(properties, true);

    // Add a request to the queue of the io-service in the rest client instance
    rest_client->Process([&](Context& ctx) {
        // Here we are again in a co-routine, running in a worker-thread.

        // Asynchronously connect to a server trough a HTTP proxy and fetch some data.
        auto reply = RequestBuilder(ctx)
            .Get("http://jsonplaceholder.typicode.com/posts/1")

            // Send the request.
            .Execute();

        // Dump the data
        cout << "Got: " << reply->GetBodyAsString();

        // Shut down the io-service. This will cause run() (below) to return.
        rest_client->CloseWhenReady();

    });

    // Start the io-service, using this thread
    rest_client->GetIoService().run();

    cout << "Done,. Exiting normally." << endl;
```



## Map between C++ property names and JSON names
TBD

## Mark properties as read-only

This is required with some REST API servers to avoid sending
properties in POST/ PUT requests that are flaghged as read-only
on the server side. (The servers will often reject such requests)

TBD

