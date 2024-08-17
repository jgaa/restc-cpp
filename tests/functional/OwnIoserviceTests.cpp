


// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"

#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/IteratorFromJsonSerializer.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;

enum { CONNECTIONS = 20 };
//#define CONNECTIONS 1

struct Post {
    string id;
    string username;
    string motto;
};

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (string, id)
    (string, username)
    (string, motto)
)

const string http_url = "http://localhost:3000/manyposts";


TEST(OwnIoservice, All)
{
    boost::asio::io_service ioservice;

    mutex mutex;
    mutex.lock();

    std::vector<std::future<int>> futures;
    std::vector<std::promise<int>> promises;

    futures.reserve(CONNECTIONS);
    promises.reserve(CONNECTIONS);

    Request::Properties properties;
    properties.cacheMaxConnections = CONNECTIONS;
    properties.cacheMaxConnectionsPerEndpoint = CONNECTIONS;

    auto rest_client = RestClient::Create(properties, ioservice);

    for(int i = 0; i < CONNECTIONS; ++i) {

        promises.emplace_back();
        futures.push_back(promises.back().get_future());

        rest_client->Process([i, &promises, &rest_client, &mutex](Context& ctx) {
            try {
                auto reply = RequestBuilder(ctx)
                    .Get(GetDockerUrl(http_url))
                    .Execute();

                // Use an iterator to make it simple to fetch some data and
                // then wait on the mutex before we finish.
                IteratorFromJsonSerializer<Post> results(*reply);

                auto it = results.begin();
                RESTC_CPP_LOG_DEBUG_("Iteration #" << i
                    << " Read item # " << it->id);

                // We can't just wait on the lock since we are in a co-routine.
                // So we use the async_wait() to poll in stead.
                while(!mutex.try_lock()) {
                    boost::asio::deadline_timer timer(rest_client->GetIoService(),
                        boost::posix_time::milliseconds(1));
                    timer.async_wait(ctx.GetYield());
                }
                mutex.unlock();

                // Fetch the rest
                for (; it != results.end(); ++it) {
                    ;
                }

                promises[i].set_value(i);
            } RESTC_CPP_IN_COROUTINE_CATCH_ALL {
                // If we got a "real" exception during the processing above
                EXPECT_TRUE(false);
                promises[i].set_exception(current_exception());
            }
        });
    }

    thread worker([&ioservice]() {
        cout << "ioservice is running" << '\n';
        ioservice.run();
        cout << "ioservice is done" << '\n';
    });

    mutex.unlock();

    int successful_connections = 0;
    for(auto& future : futures) {
        try {
            auto i = future.get();
            RESTC_CPP_LOG_DEBUG_("Iteration #" << i << " is done");
            ++successful_connections;
        } catch (const std::exception& ex) {
            std::clog << "Future threw up: " << ex.what();
        }
    }

    std::clog << "We had " << successful_connections
        << " successful connections.";

    EXPECT_EQ(CONNECTIONS, successful_connections);

    rest_client->CloseWhenReady();
    ioservice.stop();
    worker.join();
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
