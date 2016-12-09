# restc-cpp
This is a small and fast REST HTTP[S] client library in C++. It's
designed in the traditional UNIX philosophy to do one thing only,
and to do it well.

Simply said; it transforms the data in a C++ class to Json, and transmits it
to a HTTP server. It queries a HTTP server using the appropriate URL and query
arguments, receives a Json payload, and initializes a C++ object with that data.
That's it. It does not solve world hunger. It make no attempts to be a C++
framework.

You can use it's single components, like the C++ HTTP Client to send and
receive non-Json data as a native C++ replacement for libcurl.
You can use the template code that transforms data between C++ and Json
for other purposes - but the library is designed and implemented for the single
purpose of using C++ to interact efficiently and effortless with REST API servers.

The library is written by Jarle (jgaa) Aase, an enthusiastic
C++ software developer since 1996. (Before that, I used C).

# Design Goals
The design goal of this project is to make external REST API's
simple and safe to use in C++ projects, but still very fast and memory efficient
(which is why we use C++ in the first place, right?).

I also wanted to use coroutines for the application logic that sends data to or
pulls data from the REST API servers. This makes the code much easier to write
and understand, and also simplifies debugging and investigation of core dumps.
This was a strong motivation to write a C++ HTTP Client from scratch. To see how
this actually works, please see my
 [modern async cpp example](https://github.com/jgaa/modern_async_cpp_example)).

Finally, in a world where the Internet is getting increasingly
[dangerous](http://www.dailydot.com/layer8/bruce-schneier-internet-of-things/),
and all kind of malicious parties (from your own government to Russian mafia)
search for vulnerabilities in your software stack to snoop, ddos, intercept and
blackmail you and your customers/users - I have a strong emphasis on security in
all software projects I'm involved in. I have limited the dependencies on third
party libraries as much as I could (I still use OpenSSL which is a rotten pile of
shit of yet undiscovered vulnerabilities - but as of now there are no creditable
alternatives). I have also tried to imagine any possible way a malicious API server
could try to attack you (by exploiting or exceeding local resources - like sending
a malicious compressed package that expands to a petabyte of zeros) and designed
to detect any potential problems and break out of it by throwing an exception as
soon as possible - and to use fixed sized buffers in the communications layers.

# Why?
In the spring of 2016 I was tasked to implement a SDK for a REST API in
several languages. For Python, Java and Ruby it was trivial to make a simple
object oriented implementation. When I started planning the C++ implementation of the
SDK, I found no suitable, free libraries. I could not even find a proper HTTP Client
implementation(!). (I could have solved the problem using QT - but i found it
overkill to use a huge GUI framework for C++ code that are most likely to run
in high performance servers - and that may end up projects using some other
C++ framework that can't coexist with QT).

I had once, years before, done a C++ REST Client for an early
version of Amazon AWS using libcurl - and - well, I had no strong urge to repeat
that experience. So I decided to spend a week creating my own HTTP Client library
using boost::asio with Json serialization/deserialization. (Thanks to Microsoft
persistent contempt for any C++ standards, it took a little longer to finish,
as the json conversion is based on complex template meta-programming. I had some
quite beautiful code working with clang and g++, but I had to break it up and
do ugly work-arounds to make it work with MSVC. I hope I can refactor it into
boost::hana some day. However, last time I checked, Micro$oft had still not implemented
proper C++14 support - and hana was yet not working with their compiler).

In the fall / winter of 2016, I threw some more hours into the project and added
features that was not really required for the problem I started out to solve, but who
are required in a general purpose C++ REST library (like compression, HTTP redirects,
easy to use request builder, tests and demo code).

# Dependencies
Restc-cpp depends on C++14 with its standard libraries and:
  - boost
  - rapidjson (mature, ultrta-fast, json sax, header-only library)
  - unittest-cpp (If compiled with testing enabled)
  - openssl or libressl (If compiled with TLS support)
  - zlib

# License
Usually I use some version of GPL or LGPL for my projects. This
library however is so limited and general that I have released it
under the more permissive MIT license. It is Free. Free as in Free Beer.
Free as in Free Air.

# Examples

## Fetch a C++ object from a server that serialize to Json

Just to give you a teaser on how simple it is to use this library.

```C++
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

// Since C++ does not (yet) offer reflection, we need to tell the library how
// to map json members to a type. We are doing this by declaring the
// structs/classes with BOOST_FUSION_ADAPT_STRUCT from the boost libraries.
// This allows us to convert the C++ classes to and from Json.

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, userId)
    (int, id)
    (string, title)
    (string, body)
)

// The C++ main function - the place where any adventure starts
int main() {

    // Create and instantiate a Post from data received from the server.
    Post my_post = RestClient::Create()->ProcessWithPromiseT<Post>([&](Context& ctx) {
        // This is a lambda co-routine, running in a worker-thread

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

        // Return the Post instance trough a C++ future<>
        return post;
    })

    // Back in the main thread, get the Post instance from the future<>,
    // or any C++ exception thrown within the lambda.
    .get();

    // Print the result for everyone to see.
    cout << "Received post# " << my_post.id << ", title: " << my_post.title;
}
```

The code above should return something like:
```text
Received post# 1, title: sunt aut facere repellat provident occaecati excepturi optio reprehenderit
```

## Fetch raw data
You don't <i>have</i> to use futures or lambdas (although they are cool and lots of fun).
You don't even have to use the Hipster inspired RequestBuilder.

The following code demonstrates how to run a simple HTTP request asynchronously,
still using the co-routine support in boost::asio behind the scenes.


```C++
#include <iostream>
#include <chrono>
#include <thread>
#include "restc-cpp/restc-cpp.h"

using namespace std;
using namespace restc_cpp;

void DoSomethingInteresting(Context& ctx) {
    // Here we are again in a co-routine, running in a worker-thread.

    // Asynchronously connect to a server and fetch some data.
    auto reply = ctx.Get("http://jsonplaceholder.typicode.com/posts/1");

    // Asynchronously fetch the entire data-set and return it as a string.
    auto json = reply->GetBodyAsString();

    // Just dump the data.
    cout << "Received data: " << json << endl;
}

int main() {
    auto rest_client = RestClient::Create();

    // Call DoSomethingInteresting as a co-routine in a worker-thread.
    rest_client->Process(DoSomethingInteresting);

    // Wait for a little while to allow the worker-thread to finish
    this_thread::sleep_for(chrono::seconds(5));
}
```

And here is the output you could expect
```text
Received data: {
  "userId": 1,
  "id": 1,
  "title": "sunt aut facere repellat provident occaecati excepturi optio reprehenderit",
  "body": "quia et suscipit\nsuscipit recusandae consequuntur expedita et cum\nreprehenderit molestiae ut ut quas totam\nnostrum rerum est autem sunt rem eveniet architecto"
}
```

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
        .Post(""https://example.com/api/data") // URL
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


# Current Status

The code is still a bit immature and not properly tested, but capable of executing
REST requests.

The latest code is tested with Debian "testing"

Recent code was tested with Debian "stable" and "testing", Ubuntu "wily"
and Windows 10 (it should work with Windows Vista and up).

# Features
- Raw GET, POST, PUT and DELETE requests with no data conversions
- Low level interface to create requests
- Uses C++ / boost coroutines for application logic
- High level Request Builder interface (similar to Java HTTP Clients) for convenience
- Follows redirects without any extra code at the API layer
- All network IO operations are asynchronous trough boost::asio
- Logging trough boost::log or trough your own log macros
- Connection Pool for fast re-use of existing server connections.
- Compression (gzip, deflate)
- Json serialization to and from native C++ objects.
  - Optional Mapping between C++ property names and Json 'on the wire' names.
  - Option to tag property names as read-only to filter them out when the C++ object is serialized for transfer to the server.
  - Filters out empty C++ properties when the C++ object is serialized for transfer to the server (can be disabled).

# Supported development platforms:
- Linux (Debian stable and testing, Ubuntu)
- Windows 10 (Visual Studio 14 / 2015 update 2)

# Suggested target platforms:
- Linux
- OS/X
- Android (via NDK)
- Windows Vista and later

# Short Term Tasks (December 2016)
- [ ] Json Serialization / Deserialization: std::map
- Functional tests
 - [ ] Test HTTP GET (list), GET (object), POST (create), PUT (update), DELETE
 - [ ] test 1000 simultaneous sessions
- [ ] Implement asynchronous iterators for received data and integrate with json parser.
- [ ] Implement Basic Authentication
- Implement Proxy support
 - [ ] HTTP Proxy
 - [ ] Socks 5
- Portability
 - [ ] Debian Stable
 - [ ] Windows 10 / Visual Studio
 - [ ] OS/X
 - [ ] Ubuntu LTS

# Tasks planned for Q1 2017
- Performance analysis and optimizations for speed and memory footprint
- Refactor
 - [ ] split the json serialization and HTTP client into independent sub-projects
- Implement Chunked Requests (chained DataWriter interface)
 - [ ] General support In HTTP Requests module
 - [ ] Async from json Serialization
 - [ ] Async from producer callback
 - [ ] Async from producer loop
 - [ ] Implement asynchronous iterators for outgoing data and integrate with json generator.
- Improve security
 - [ ] Put memory constraints on strings and lists in the json deserialization
 - [ ] Add options to secure TLS connections (avoid weak encryption and verify server certs).
- Implement Form Data encoding
- Portability
 - [ ] Windows 10 / clang
 - [ ] Cent OS
 - [ ] Fedora
 - [ ] FreeBSD
 - [ ] OpenBSD


# Future maybe someday features
- Json
 - std::set
 - True generic container support (any object that support forward iteration and insert/add)
- Add data-type suitable for representing money (must be able to serialize deserialize like float/ BigDecimal)
- Mime content in HTTP body
 - Mime multipart Requests
 - Mime multipart Responses
- Circuit Breaker (Fail fast for hosts that don't work)
- Bulkheads (Use separate connection pools for different services)
- Make performance comparisons with similar REST libraries for Java, Python and Ruby
