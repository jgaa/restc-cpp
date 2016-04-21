# restc-cpp

This is a minimalistic but still functioning and very fast
(at least, that is the intention) HTTP/HTTPS client library in C++.

It depends on C++14 with its standard libraries and boost.
It uses boost::asio for IO.

The library is written by Jarle (jgaa) Aase, an enthusiastic
C++ software developer since 1996. (Before that, I used C).

## Design Goal
The design goal of this project is to make external REST API's
simple to use in C++ projects, but still very fast (which is why
we use C++ in the first place, right?).

Usually I use some version of GPL or LGPL for my projects. This
library however is so tiny and general that I have released it
under the more permissive MIT license. It is Free. Free as in Free Beer.
Free as in Free Air.

## Examples

### Fetch raw data

The following code is all that is needed to run REST requests asynchronously,
using the co-routine support in boost::asio behind the scenes. (To see how
this works, please see my
[modern async cpp example](https://github.com/jgaa/modern_async_cpp_example)).


```C++
#include <iostream>
#include "restc-cpp/restc-cpp.h"

using namespace std;
using namespace restc_cpp;

void DoSomethingInteresting(Context& ctx) {

    // Asynchronously connect to a server and fetch some data.
    auto reply = ctx.Get("http://jsonplaceholder.typicode.com/posts");

    // Asynchronously fetch the entire data-set and return it as a string.
    auto json = reply->GetBodyAsString();

    // Just dump the data.
    clog << "Received data: " << json << endl;
}

main(int argc, char *argv[]) {
    auto rest_client = RestClient::Create();

    /* Call DoSomethingInteresting as a co-routine in a worker-thread.
     * This version of Proces*() returns a future.
     */
    auto future = rest_client->ProcessWithPromise(DoSomethingInteresting);

    // Hold the main thread...
    future.wait();
}
```


### Fetch json data and convert to native C++ types.

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

Since C++ does not (yet) offer reflection, we need to tell the library how
to map json members to a type. We are doing this by declaring the
structs/classes with BOOST_FUSION_ADAPT_STRUCT from the boost libraries.

```C++
#include <iostream>

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

        clog << "Done" << endl;

    } catch (const exception& ex) {
        clog << "Process: Caught exception: " << ex.what() << endl;
    }
}

```

## Current Status

The code is still a bit immature and not properly tested, but capable of executing
REST requests.

The latest code is tested with Debian "stable" and "testing", and Ubuntu "wily".

The library compiles and passes all tests under Windows as well, except that
TLS is currently not working with boost 1.60.0 and OpenSSL 1.1.0. That
means that https support is broken under Windows at the moment.


## Supported development platforms:
- Linux (Debian stable and testing, Ubuntu)
- Windows 10 (Latest "community" C++ compiler from Microsoft)

## Suggested target platforms:
- Linux
- OS/X
- Android (via NDK)
- Windows Vista and later


## Short Term Tasks
- [x] ~~Implement GET, POST, PUT, DELETE~~
- [x] ~~Implement HTTPS~~
- [ ] Implement proper logging
- [ ] Json support (work in progress)
 - [x] ~~Deserialization: Simple and nested classes (must be declared with BOOST_FUSION_ADAPT_STRUCT)~~
 - [x] ~~Deserialization: std::vector of json native datatypes and classes~~
 - [x] ~~Serialization of the above~~
 - [ ] Serialization / Deserialization: std::map
 - [ ] Serialization / Deserialization: std::deque
- [ ] Unit tests
 - [x] ~~Url parser~~
 - [x] ~~Json / C++ transformation~~
 - [ ] HTTP header parser
 - [ ] HTTP Chunked repsponse / odd cases
- [ ] Implement connection pool
- [ ] Implement Chunked Reponse handling
 - [x] ~~Handle normal use-cases~~
 - [ ] Handle trailers
- [ ] Implement Chunked Requests
 - [ ] General support In HTTP Requests module
 - [ ] Async from json Serialization
- [ ] Handle redirects
- [x] ~~Implement simple File Upload (as body)~~
- [ ] Implement simple File Download (from body)
- [x] ~~Verify that it compiles with Debian Stable~~
- [ ] Verify that it compiles with Windows 10 / Visual Studio
 - [x] Library and json, unittests, functional tests
 - [ ] https (boost::asio/tls 1.60 not compiling with the latest openssl)
- [ ] Verify that it compiles with OS/X
- [ ] Implement Mime multipart Requests
- [ ] Implement Mime multipart Responses
- [ ] Implement Form Data encoding (with File Upload)
- [ ] Implement asynchronous iterators for received data and integrate with json parser.
- [ ] Implement asynchronous iterators for requests data and integrate with json generator.
- [ ] Add options to secure TLS connections (avoid weak encryption and verify server certs).
- [ ] Add compression for the IO stream

