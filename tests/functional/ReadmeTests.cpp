#include <iostream>

#define RESTC_CPP_ENABLE_URL_TEST_MAPPING 1

#include <boost/fusion/adapted.hpp>
#include <boost/lexical_cast.hpp>
#include <utility>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/IteratorFromJsonSerializer.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"
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

string proxy_address = GetDockerUrl("http://localhost:3003");

// The C++ main function - the place where any adventure starts
void first() {

    // Create an instance of the rest client
    auto rest_client = RestClient::Create();

    // Create and instantiate a Post from data received from the server.
    Post const my_post = rest_client->ProcessWithPromiseT<Post>([&](Context& ctx) {
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
    cout << "Received data: " << json << '\n';
}

void second() {
    auto rest_client = RestClient::Create();

    // Call DoSomethingInteresting as a co-routine in a worker-thread.
    rest_client->Process(DoSomethingInteresting);

    rest_client->CloseWhenReady();
}

void third() {

    RestClient::Create()->ProcessWithPromise([&](Context& ctx) {
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
    properties.proxy.address = proxy_address;

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
}

void fifth() {
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
            cout << "Item #" << post.id << " Title: " << post.title << '\n';
        }
    });
}

void sixth() {
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

    cout << "Done. Exiting normally." << '\n';
}

// Use our own RequestBody implementation to supply
// data to a POST request
void seventh() {
    // Our own implementation of the raw data provider
    class MyBody : public RequestBody
    {
    public:
        MyBody() = default;

        [[nodiscard]] Type GetType() const noexcept override {

            // This mode causes the request to use chunked data,
            // allowing us to send data without knowing the exact
            // size of the payload when we start.
            return Type::CHUNKED_LAZY_PULL;
        }

        [[nodiscard]] std::uint64_t GetFixedSize() const override {
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
}

struct DataItem {
    DataItem() = default;
    DataItem(string u, string m)
        : username{std::move(u)}
        , motto{std::move(m)}
    {}

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

void eight() {

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


void ninth() {

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
}

void tenth() {
    // Construct our own ioservice.
    boost::asio::io_service ioservice;

    // Give it some work so it don't end prematurely
    boost::asio::io_service::work const work(ioservice);

    // Start it in a worker-thread
    thread worker([&ioservice]() {
        cout << "ioservice is running" << '\n';
        ioservice.run();
        cout << "ioservice is done" << '\n';
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
        cout << "Received data: " << json << '\n';
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

    cout << "Done." << '\n';
}

void eleventh() {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        Post data;
        data.id = 10;
        data.userId = 14;
        data.title = "Hi there";
        data.body = "This is the body.";

        excluded_names_t const exclusions{"id", "userId"};

        auto reply = RequestBuilder(ctx)
            .Post("http://localhost:3000/posts")

            // Suppress sending 'id' and 'userId' properties
            .Data(data, nullptr, &exclusions)

            .Execute();
    }).get();
}

void twelfth() {
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
}

void thirtheenth() {
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
}

void fourteenth() {
    Request::Properties properties;
    //properties.bindToLocalAddress = "127.0.0.1:";
    properties.bindToLocalAddress = ":12345";
    //properties.bindToLocalAddress = "[::1]:12345";
    //properties.bindToLocalAddress = "localhost:12345";
    auto rest_client = RestClient::Create(properties);

    // Call DoSomethingInteresting as a co-routine in a worker-thread.
    rest_client->Process(DoSomethingInteresting);
}

TEST(ReadmeTests, All) {
    cout << "First: " << '\n';
    EXPECT_NO_THROW(first());

    cout << "Second: " << '\n';
    EXPECT_NO_THROW(second());

    cout << "Third: " << '\n';
    EXPECT_NO_THROW(third());

    cout << "Forth: " << '\n';
    EXPECT_NO_THROW(forth());

    cout << "Fifth: " << '\n';
    EXPECT_NO_THROW(fifth());

    cout << "Sixth: " << '\n';
    EXPECT_NO_THROW(sixth());

    cout << "Seventh: " << '\n';
    EXPECT_NO_THROW(seventh());

    cout << "Eight: " << '\n';
    EXPECT_NO_THROW(eight());

    cout << "Ninth: " << '\n';
    EXPECT_NO_THROW(ninth());

    cout << "Tenth: " << '\n';
    EXPECT_NO_THROW(tenth());

    cout << "Eleventh: " << '\n';
    EXPECT_NO_THROW(eleventh());

    cout << "Twelfth: " << '\n';
    EXPECT_NO_THROW(twelfth());

    cout << "Thirtheenth: " << '\n';
    EXPECT_NO_THROW(thirtheenth());

    cout << "Fourteenth: " << '\n';
    EXPECT_NO_THROW(fourteenth());
}

int main(int argc, char * argv[]) {
    RESTC_CPP_TEST_LOGGING_SETUP("info");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
