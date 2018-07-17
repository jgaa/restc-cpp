# Tutorial

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
receive really large lists, this may not be a good idea, as your RAM would fill
up and eventually your application would hang or die.

Another, much better approach, is to fetch the list into a C++ input iterator.
In this case, we only store one data item in memory at any time (well, two
if you use the <i>it++</i> operator - something you only do when you really
need the data before the increment - right?). The size of the list does not
matter. The HTTP client will fetch data asynchronously in the background
when it is parsing the json stream to instantiate a data object. When an
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

## Add arguments to the request-url

Arguments in the URL require some special care. In most situations you should
use the *RequestBuilder*'s *Argument()* method for this.

```C++
    // Create an instance of the rest client
    auto rest_client = RestClient::Create();

    // Create a coroutine to send our request
    rest_client->ProcessWithPromise([&](Context& ctx) {
        // This is a co-routine, running in a worker-thread

        // Construct a request to the server
        auto reply = RequestBuilder(ctx)
                .Get("https://www.google.com/search")

                /* Add some request arguments
                 * Here we use Google search with arguments:
                 *
                 *   hl=en
                 *   q=site:lastviking.eu+stbl
                 *
                 * which tells Google to search for 'stbl' at the site lastviking.eu,
                 * using the English language.
                 *
                 * (I believe Google is recruiting their product-owners from Hell.
                 * One of their more annoying treats is to use geo-location in
                 * stead of browser-settings to determine the language of the
                 * Google web pages.
                 * Ever used Google's services from Russia? Unless you read
                 * Russian - just don't! Or add the 'hl' argument (which is
                 * probably what the Google developers do when they travel)
                 */

                .Argument("hl", "en")
                .Argument("q", "site:lastviking.eu+stbl")

                // Send the request
                .Execute();

        // Fetch the payload asynchronously and send it to standard output
        std::cout << reply->GetBodyAsString();

        // Return the post instance trough a C++ future<>
        return ;
    });

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

## Use an existing thread in stead of a new worker thread

This example is slightly more advanced. Here we take
responsibility to run the io-service used internally by
the rest client instance. This allow us to use an existing
thread, like the applications main thread, to handle the
asynchronous requests.

```C++
   // Create the client without creating a worker thread
    auto rest_client = RestClient::CreateUseOwnThread();

    // Add a request to the queue of the io-service in the rest client instance
    rest_client->Process([&](Context& ctx) {
        // Here we are again in a co-routine, now in our own thread.

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

    // Start the io-service, using this thread.
    rest_client->GetIoService().run();

    cout << "Done. Exiting normally." << endl;
```

## Use an existing asio io-service

If you use this library in a server, you may already use boost::asio for
network IO. If you want to avoid adding more threads (for example by
limiting the number of threads to the number of cores on the machine - a
strategy that can give an extreme performance), you can use your existing
io-services.

Note that currently, restc-cpp require that it's io-service use only one
thread. (With asio, you can use two strategies, one io-service and
several threads, or several io-services, each using only one thread.
I have favored the one-thread approach as it makes the code simpler, and
theoretically, it should make less CPU cache misses, something that
should further improve the performance).

```C++
    // Construct our own ioservice.
    boost::asio::io_service ioservice;

    // Give it some work so it don't end prematurely
    boost::asio::io_service::work work(ioservice);

    // Start it in a worker-thread
    thread worker([&ioservice]() {
        cout << "ioservice is running" << endl;
        ioservice.run();
        cout << "ioservice is done" << endl;
    });

    // Now we have our own io-service running in a worker thread.
    // Create a RestClient instance that uses it.

    auto rest_client = RestClient::Create(ioservice);

    // Make a HTTP request
    rest_client->ProcessWithPromise([&](Context& ctx) {
        // Here we are in a co-routine, spawned from our io-service, running
        // in our worker thread.

        // Asynchronously connect to a server and fetch some data.
        auto reply = ctx.Get("http://jsonplaceholder.typicode.com/posts/1");

        // Asynchronously fetch the entire data-set and return it as a string.
        auto json = reply->GetBodyAsString();

        // Just dump the data.
        cout << "Received data: " << json << endl;
    })
    // Wait for the co-routine to end
    .get();

    // Stop the io-service
    // (We can not just remove 'work', as the rest client may have
    // timers pending in the io-service, keeping it running even after
    // work is gone).
    ioservice.stop();

    // Wait for the worker thread to end
    worker.join();

    cout << "Done." << endl;
```

## Use your own data provider to feed data into a POST request
```C++
    // Our own implementation of the raw data provider
    class MyBody : public RequestBody
    {
    public:
        MyBody() = default;

        Type GetType() const noexcept override {

            // This mode causes the request to use chunked data,
            // allowing us to send data without knowing the exact
            // size of the payload when we start.
            return Type::CHUNKED_LAZY_PULL;
        }

        std::uint64_t GetFixedSize() const override {
            throw runtime_error("Not implemented");
        }

        // This will be called until we return false to indicate
        // that we have no further data
        bool GetData(write_buffers_t& buffers) override {

            if (++count_ > 10) {

                // We are done.
                return false;
            }

            ostringstream data;
            data << "This is line #" << count_ << " of the payload.\r\n";

            // The buffer need to persist until we are called again, or the
            // instance is destroyed.
            data_buffer_ = data.str();

            buffers.emplace_back(data_buffer_.c_str(), data_buffer_.size());

            // We added data to buffers, so return true
            return true;
        }

        // Called if we get a HTTP redirect and need to start over again.
        void Reset() override {
            count_ = 0;
        }

    private:
        int count_ = 0;
        string data_buffer_;
    };


    // Create the REST clent
    auto rest_client = RestClient::Create();

    // Run our example in a lambda co-routine
    rest_client->Process([&](Context& ctx) {
        // This is the co-routine, running in a worker-thread

        // Construct a POST request to the server
        RequestBuilder(ctx)
            .Post("http://localhost:3001/upload_raw/")
            .Header("Content-Type", "text/text")
            .Body(make_unique<MyBody>())
            .Execute();
    });


    // Wait for the request to finish
    rest_client->CloseWhenReady(true);

```

## Serialize an outgoing json list of objects directly to server

In this example, we will serialize a list of json objects at the
last possible moment, and without going trough any temporary
buffers. We will write directly to the DataWriter for the outgoing
request. In real life, this can be used to dump large data-sets
to a server, like database tables or log events.

```C++
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"


struct DataItem {
    DataItem() = default;
    DataItem(string u, string m)
    : username{u}, motto{m} {}

    int id = 0;
    string username;
    string motto;
};

BOOST_FUSION_ADAPT_STRUCT(
    DataItem,
    (int, id)
    (string, username)
    (string, motto)
)

int main() {
     // Create the REST clent
    auto rest_client = RestClient::Create();

    // Run our example in a lambda co-routine that returns a future
    rest_client->ProcessWithPromise([&](Context& ctx) {

        // Make a container for data
        std::vector<DataItem> data;

        // Add some data
        data.emplace_back("jgaa", "Carpe Diem!");
        data.emplace_back("trump", "Endorse greed!");
        data.emplace_back("anonymous", "We are great!");

        // Create a request
        auto reply = RequestBuilder(ctx)
            .Post("http://localhost:3001/upload_raw/") // URL

            // Provide data from a lambda
            .DataProvider([&](DataWriter& writer) {
                // Here we are called from Execute() below to provide data

                // Create a json serializer that can write data asynchronously
                RapidJsonInserter<DataItem> inserter(writer, true);

                // Serialize the items from our data container
                for(const auto& d : data) {

                    // Serialize one data item.
                    // If the buffers in the writer fills up, we will
                    // write data to the net asynchronously.
                    inserter.Add(d);
                }

                // Tell the inserter that we have no further data.
                inserter.Done();

            })

            // Execute the request
            .Execute();

    })

    // Wait for the request to finish.
    .get();
}
```

## Serialize an outgoing json list of objects directly to server manually

Until now, when we have used the RequestBuilder, we have finalized the
request with Execute(). This is normally what we want to do, as it takes
care of the different stages of the request, and also handles
redirects.

However, there may be use-cases where we want more control. In the
example below, we will send the request, serialize some data, and
then wait for the reply.

```C++
    // Create the REST clent
    auto rest_client = RestClient::Create();

    // Run our example in a lambda co-routine that returns a future
    rest_client->ProcessWithPromise([&](Context& ctx) {

        // Make a container for data
        std::vector<DataItem> data;

        // Add some data
        data.emplace_back("jgaa", "Carpe Diem!");
        data.emplace_back("trump", "Endorse greed!");
        data.emplace_back("anonymous", "We are great!");

        // Prepare the request
        auto request = RequestBuilder(ctx)
            .Post("http://localhost:3001/upload_raw/") // URL

            // Make sure we get a DataWriter for chunked data
            // This is required when we add data after the request-
            // headers are sent.
            .Chunked()

            // Just create the request. Send nothing to the server.
            .Build();

        // Send the request to the server. This will send the
        // request line and the request headers.
        auto& writer = request->SendRequest(ctx);

        {
            // Create a json list serializer for our data object.
            RapidJsonInserter<DataItem> inserter(writer, true);

            // Write each item to the server
            for(const auto& d : data) {
                inserter.Add(d);
            }
        }

        // Finish the request and fetch the reply asynchronously
        // This function returns when we have the reply headers.
        // If we expect data in the reply, we can read it asynchronously
        // as shown in previous examples.
        auto reply = request->GetReply(ctx);

        cout << "The server replied with code: " << reply->GetResponseCode();

    })

    // Wait for the request to finish.
    .get();
```

## Map between C++ property names and JSON names
Some times you may have different field names in C++ and Json.

```C++
struct User {
    string user_id;
    string user_name;
    int age;
};
```

```json
{
    "userId": ...,
    "userName": ...,
    "age": ...
}

```

In such cases, you can map the names. The mapping applies during
serialization, and must therefore be applied to the serializer you
use.

If you use the RequestBuilder to serialize an outgoing C++ object,
you can do like this:

```C++
    JsonFieldMapping mapping;
    mapping.entries.emplace_back("user_id", "userId");
    mapping.entries.emplace_back("user_name", "userName");

    auto reply = RequestBuilder(ctx)
        .Post("https://example.com/api/data")              // URL
        .Header("X-Client", "RESTC_CPP")                   // Optional header

        // Apply the mapper for name mapping
        .Data(data_object, &mapper)                        // Data object to send
        .Execute();
```

You can apply the same mapping (via member functions) to
- RapidJsonInserter
- RapidJsonSerializer
- SerializeFromJson


```C++
    auto reply = RequestBuilder(ctx)
        .Post("http://localhost:3001/upload_raw/") // URL

        .DataProvider([&](DataWriter& writer) {
            RapidJsonInserter<DataItem> inserter(writer, true);

            // Apply name mapping
            inserter.SetNameMapping(&mapping)

            for(const auto& d : data) {
                inserter.Add(d);
            }
            inserter.Done();

        })
        .Execute();
```

## Mark properties as read-only

This is required with some REST API servers to avoid sending
properties in POST/ PUT requests that are flagged as read-only
on the server side. (The servers will often reject such requests)

Simply supply a list of censored property-names.

```C++
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        Post data;
        data.id = 10;
        data.userId = 14;
        data.title = "Hi there";
        data.body = "This is the body.";

        excluded_names_t exclusions{"id", "userId"};

        auto reply = RequestBuilder(ctx)
            .Post("http://localhost:3000/posts")

            // Suppress sending 'id' and 'userId' properties
            .Data(data, nullptr, &exclusions)

            .Execute();
    }).get();
```
You can apply the same restriction (via method calls) to
- RapidJsonInserter
- RapidJsonSerializer

## Setting a memory limit on incoming objects

It is possible for a rogue REST server to attack connected clients
by sending a never-ending stream of valid json data (for example a
list that never ends, or a string of unlimited size). To prevent such
attacks, restc-cpp has a default limit on 1 megabyte for objects
that are serialized from json. (The limit is based on an approximate
memory usage, not the exact memory allocated by the operation).

This feature can be fine tuned (for example by setting the limit to
2 kilobytes for a object that is expected to be small, or disabled
if you are running it on a supercomputer, or if you trust the REST
server 100%.

Example on how to use it:
```C++
    auto rest_client = RestClient::Create();
    RestClient::Create()->ProcessWithPromise([&](Context& ctx) {
        Post post;

        // Serialize with a limit of 2 kilobytes of memory usage by the post;
        SerializeFromJson(post,
            RequestBuilder(ctx)
                .Get("http://jsonplaceholder.typicode.com/posts/1")

                // Notice the limit of 2048 bytes
                .Execute(), nullptr, 2048);

        // Serialize with no limit;
        SerializeFromJson(post,
            RequestBuilder(ctx)
                .Get("http://jsonplaceholder.typicode.com/posts/1")

                // Notice how we disable the constraint by defining zero
                .Execute(), nullptr, 0);
    })
    .get();
```

## Serializing a file (or any std::istream) with json data to a C++ object

I tend to use json more and more for configuration files.
Boost.ProgramOptions is great for this (and it supports several
other input formats). However, it's not exactly easy to use.
Some times, for simple configurations, it can be easier to
just serialize directly to a C++ object.

This is one of many use-cases for istream2json serialization.

```C++
#include <cstdio>
#include <boost/fusion/adapted.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"

using namespace std;
using namespace restc_cpp;

// Our configuration object
struct Config {
    int max_something = {};
    string name;
    string url;
};

// Declare Config to boost::fusion, so we can serialize it
BOOST_FUSION_ADAPT_STRUCT(
    Config,
    (int, max_something)
    (string, name)
    (string, url)
)

main() {

    // Create an istream for the json file
    ifstream ifs("config.json");

    // Instatiate the config object
    Config config;

    // Read the ;config file into the config object.
    SerializeFromJson(config, ifs);

    // Do something with config...
}

```

Note that serializing from std::iostream does not give
the absolutely best performance rapidjson and restc-cpp can
achieve. It is however the most flexible implementation.

Please file a ticket on github if you require a native file reader
with optimal performance. (Don't worry - I won't charge you - I just
focus on the most useful features that people actually need).

## Serializing a C++ object to a file (or any std::ostream)

```C++
#include <cstdio>
#include <boost/fusion/adapted.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"

using namespace std;
using namespace restc_cpp;

// Our configuration object
struct Config {
    int max_something = {};
    string name;
    string url;
};

// Declare Config to boost::fusion, so we can serialize it
BOOST_FUSION_ADAPT_STRUCT(
    Config,
    (int, max_something)
    (string, name)
    (string, url)
)

main() {

    // Create an ostream for the json file
    ofstream ofs("config.json");

    // Instatiate the config object
    Config config;

    // Assign some values
    config.max_something = 100;
    config.name = "John";
    config.url = "https://www.example.com";

    // Serialize Config to the file
    SerializeToJson(config, ofs);
}

```

