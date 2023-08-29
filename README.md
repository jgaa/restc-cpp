# Introduction to the restc-cpp C++ library
<i>The magic that takes the pain out of accessing JSON API's from C++ </i>

<b>What it does:</b>
- It formulates a HTTP request to a REST API server. Then, it transforms
  the JSON formatted payload in the reply into a native C++ object (GET).
- It Serialize a native C++ object or a container of C++ objects into a JSON payload
  and send it to the REST API server (POST, PUT).
- It formulates a HTTP request to the REST API without serializing any data in either
  direction (typically DELETE).
- It uploads a stream of data, like a file, to a HTTP server.
- It downloads a stream of data, like a file or an array of JSON objects, from a HTTP server.

That's basically it. It does not solve world hunger.
It make no attempts to be a C++ framework.

You can use it's single components, like the powerful C++ HTTP Client to
send and receive non-JSON data as a native C++ replacement for libcurl.
You can use the template code that transforms data between C++ and JSON
for other purposes (for example in a REST API SERVER) - but the library
is designed and implemented for the single purpose of using C++ to
interact efficiently and effortless with external REST API servers.

The library is written by Jarle (jgaa) Aase, a senior freelance C++ developer
with roughly 30 years of experience in software development.

# Design Goals
The design goal of this project is to make external REST API's
simple and safe to use in C++ projects, but still fast and memory efficient.

Another goal was to use coroutines for the application logic that sends data to or
pulls data from the REST API servers. This makes the code easy to write
and understand, and also simplifies debugging and investigation of core dumps.
In short; the code executes asynchronously, but there are no visible callbacks
or completion functions. It looks like crystal clear,
old fashion, single threaded sequential code (using modern C++ language).
You don't sacrifice code clearness to achieve massive parallelism and
high performance. Coroutines was a strong motivation to write a new
C++ HTTP Client from scratch. To see how this actually works, please see the
 [modern async cpp example](https://github.com/jgaa/modern_async_cpp_example)).


Finally, in a world where the Internet is getting increasingly
[dangerous](http://www.dailydot.com/layer8/bruce-schneier-internet-of-things/),
and all kind of malicious parties, from your own government to the international Mafia
(with Putin in Moscow and other autocrats in parliaments and as head of state all over the world - 
including USA, EU and Norway -, the differences is
blurring out), search for vulnerabilities in your software stack to snoop, ddos,
intercept and blackmail you and your customers/users - I have a strong emphasis
on security in all software projects I'm involved in. I have limited the
dependencies on third party libraries as much as I could (I still use OpenSSL
which is a snakes nest of of yet undisclosed vulnerabilities - but as of now
there are no alternatives that works out of the box with boost::asio).
I have also tried to imagine any possible way a malicious API server
could try to attack you (by exploiting or exceeding local resources - like sending
a malicious compressed package that expands to a petabyte of zeros) and designed
to detect any potential problems and break out of it by throwing an exception as
soon as possible.

# Why?
In the spring of 2016 I was asked to implement a SDK for a REST API in
several languages. For Python, Java and Ruby it was trivial to make a simple
object oriented implementation. When I started planning the C++ implementation of the
SDK, I found no suitable, free libraries. I could not even find a proper HTTP Client
implementation(!). I could have solved the problem using QT - but i found it
overkill to use a huge GUI framework for C++ code that are most likely to run
in high performance servers - and that may end up in projects using some other
C++ framework that can't coexist with QT.

Many years ago I designed and implemented a C++ REST Client for an early
version of Amazon AWS using libcurl - and - well, I had no strong urge to repeat
that experience. So I spent a few weeks creating my own HTTP Client library
using boost::asio with JSON serialization/deserialization.

# Dependencies
Restc-cpp depends on C++14 with its standard libraries and:
  - boost
  - rapidjson (CMake will download and install rapidjson for the project)
  - gtest (CMake will download and install gtest for the project if it is not installed)
  - openssl or libressl (If compiled with TLS support)
  - zlib (If compiled with compression support)

# License
MIT license. It is Free. Free as in speech. Free as in Free Air.

# Examples

## Fetch raw data
The following code demonstrates how to run a simple HTTP request asynchronously,
using the co-routine support in boost::asio behind the scenes.


```C++
#include <iostream>
#include "restc-cpp/restc-cpp.h"

using namespace std;
using namespace restc_cpp;

void DoSomethingInteresting(Context& ctx) {
    // Here we are in a co-routine, running in a worker-thread.

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

    // Wait for the coroutine to finish, then close the client.
    rest_client->CloseWhenReady(true);
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


## Fetch a C++ object from a server that serialize to JSON

Here is a sightly more interesting example, using JSON
serialization, and some modern C++ features.

```C++
#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

using namespace std;
using namespace restc_cpp;

// C++ structure that match the JSON entries received
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
// This allows us to convert the C++ classes to and from JSON.

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, userId)
    (int, id)
    (string, title)
    (string, body)
)

// The C++ main function - the place where any adventure starts
int main() {
    // Create an instance of the rest client
    auto rest_client = RestClient::Create();

    // Create and instantiate a Post from data received from the server.
    Post my_post = rest_client->ProcessWithPromiseT<Post>([&](Context& ctx) {
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
```

The code above should return something like:
```text
Received post# 1, title: sunt aut facere repellat provident occaecati excepturi optio reprehenderit
```

Please refer to the [tutorial](doc/Tutorial.md) for more examples.

# Features
- High level Request Builder interface (similar to Java HTTP Clients) for convenience.
- Low level interface to create requests.
- All network IO operations are asynchronous trough boost::asio.
  - Use your own asio io-services
  - Let the library create and deal with the asio io-services
  - Use your own worker threads
  - Let the library create and deal with worker-threads
- Uses C++ / boost coroutines for application logic.
- HTTP Redirects.
- HTTP Basic Authentication.
- [Logging](doc/Logging.md) trough logfault, boost::log, std::clog or trough your own log macros or via a callback to whatever log framework you use.
- Log-level for the library can be set at compile time (none, error, warn, info, debug, trace)
- Connection Pool for fast re-use of existing server connections.
- Compression (gzip, deflate).
- JSON serialization to and from native C++ objects.
  - Optional Mapping between C++ property names and JSON 'on the wire' names.
  - Option to tag property names as read-only to filter them out when the C++ object is serialized for transfer to the server.
  - Filters out empty C++ properties when the C++ object is serialized for transfer to the server (can be disabled).
  - Iterator interface to received JSON lists of objects.
  - Memory constraint on incoming objects (to limit damages from rouge or buggy REST servers).
  - Serialization directly from std::istream to C++ object.
- Plain or chunked outgoing HTTP payloads.
- Several strategies for lazy data fetching in outgoing requests.
  - Override RequestBody to let the library pull for data when required.
  - Write directly to the outgoing DataWriter when data is required.
  - Just provide a C++ object and let the library serialize it directly to the wire.
- HTTP Proxy support
- SOCKS5 Proxy support (naive implementatin for now, no support for authentication).

# Current Status
The project has been in public BETA since April 11th 2017.

# Supported operating systems
These are the operating systems where my Continues Integration (Jenkins) servers currently compiles the project and run all the tests:

 - Debian Testing
 - Debian Bookworm
 - Debian Bullseye (Stable)
 - Debian Buster
 - Windows 10 / Microsoft Visual Studio 2019, Community version using vcpkg for dependencies
 - Ubuntu Xenial (LTS)

Support for MacOS has been removed after Apples announcement that their love for privacy was just 
a marketing gimmick.
 
Fedora is currently disabled in my CI because of failures to start their Docker containers. (Work in progress). Ubuntu Jammy don't work in docker with my Jenkins  CI pipeline, so I have no reliable way to test it. Windows 11 cannot be run on my KVM /QEMU system, because it don't support "secure" boot, so I have no way to test it.

The Jenkins setup is [here](ci/jenkins).

I currently use my own CI infrastructure running on my own hardware. I use Jenkins on a VM with Debian Bullseye, and three slaves for Docker on Linux VM's, one slave running on a VM with Microsoft Windows 10 Pro. Using Docker to build with different Linux distributions gives me flexibility. It also immediately catches mistakes that break the build or test(s) on a specific Linux distribution or platform. Using my own infrastructure improves the security, as I don't share any credentials with 3rd party services or allow external access into my LAN.

# Blog-posts about the project:
  - [About version 0.90](https://lastviking.eu/restc_cpp_90.html)
  - [restc-cpp tags on The Last Viking's Nest](https://lastviking.eu/_tags/restc-cpp.html)

# Similar projects
  - [RESTinCurl](https://github.com/jgaa/RESTinCurl) by me. Aimed at mobile applications, IoT and projects that already link with libcurl.
  - [Boost.Beast](https://github.com/boostorg/beast) by Vinnie Falco. When you like to write many lines of code...

  **Json serialization only**
  - [Boost.Json](https://www.boost.org/doc/libs/1_83_0/libs/json/doc/html/index.html)
  - [JSON for Modern C++](https://nlohmann.github.io/json/) by Niels Lohmann. My favorite json library, when I need to more than just static serialization.
  - [json11 - tiny JSON library for C++11, providing JSON parsing and serialization](https://github.com/dropbox/json11)

# More information
- [Getting started](doc/GettingStarted.md)
- [Tutorial](doc/Tutorial.md)
- [Build for Linux](https://github.com/jgaa/restc-cpp/wiki/Building-under-Linux)
- [Build for macOS](https://github.com/jgaa/restc-cpp/wiki/Building-under-macOS)
- [Build for Windows](https://github.com/jgaa/restc-cpp/wiki/Building-under-Windows)
- [Running the tests](doc/RunningTheTests.md)
- [Planned work](https://github.com/jgaa/restc-cpp/wiki)
- [How to link your program with restc-cpp from the command-line](https://github.com/jgaa/restc-cpp/tree/master/examples/cmdline)
- [How to use cmake to find and link with restc-cpp from your project](https://github.com/jgaa/restc-cpp/tree/master/examples/cmake_normal)
- [How to use restc-cpp as an external cmake module from your project](https://github.com/jgaa/restc-cpp/tree/master/examples/cmake_external_project)
