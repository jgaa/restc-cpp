


// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/IteratorFromJsonSerializer.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;


const string http_url = "http://localhost:3000/manyposts";

/* The goal is to test with 1000 connections.
 * However, I am unable to get more than 500 working reliable (with 100
 * connections increment) before I see connection errors. On OS X,
 * I was unable to get more than 100 connections working reliable.
 * (I have yet to figure out if the limitation is in the library
 * or in the test setup / Docker container).
 *
 * I don't know at this time if it is caused by OS limits in the test
 * application, Docker, the container with the mock server or the Linux
 * machine itself.
 *
 * May be I have to write a simple HTTP Mock sever in C++ or use
 * nginx-lua with some tweaking / load-balancing to get this test
 * to work with the 1000 connection goal.
 *
 * There is also a problem where several tests hit's the test containers
 * from Jenkins. So for now 100 connections must suffice.
 *
 * 100 connections is sufficient to prove that the client
 * works as expected with many co-routines in parallel.
 */

#define CONNECTIONS 100

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

struct Locker {
    Locker(mutex& m) : m_{m} {}
    ~Locker() {
        if (locked_) {
            m_.unlock();
        }
    }

    bool try_lock() {
        assert(!locked_);
        locked_ = m_.try_lock();
        return locked_;
    }

    void unlock() {
        assert(locked_);
        m_.unlock();
        locked_ = false;
    }

    mutex& m_;
    bool locked_{};
};

TEST(ManyConnections, CRUD) {
    mutex mutex;
    mutex.lock();

    std::vector<std::future<int>> futures;
    std::vector<std::promise<int>> promises;

    futures.reserve(CONNECTIONS);
    promises.reserve(CONNECTIONS);

    Request::Properties properties;
    properties.cacheMaxConnections = CONNECTIONS;
    properties.cacheMaxConnectionsPerEndpoint = CONNECTIONS;
    auto rest_client = RestClient::Create(properties);

    for(int i = 0; i < CONNECTIONS; ++i) {

        promises.emplace_back();
        futures.push_back(promises.back().get_future());

        rest_client->Process([i, &promises, &rest_client, &mutex](Context& ctx) {
            Locker locker(mutex);
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

                promises[i].set_value(i);
                // Wait for all connections to be ready

                // We can't just wait on the lock since we are in a co-routine.
                // So we use the async_wait() to poll in stead.
                while(!locker.try_lock()) {
                    boost::asio::deadline_timer timer(rest_client->GetIoService(),
                        boost::posix_time::milliseconds(1));
                    timer.async_wait(ctx.GetYield());
                }
                locker.unlock();

                // Fetch the rest
                for(; it != results.end(); ++it)
                    ;

            } catch (const std::exception& ex) {
                RESTC_CPP_LOG_ERROR_("Failed to fetch data: " << ex.what());
                promises[i].set_exception(std::current_exception());
            }
        });
    }

    int successful_connections = 0;
    for(auto& future : futures) {
        try {
            auto i = future.get();
            RESTC_CPP_LOG_DEBUG_("Iteration #" << i << " is done");
            ++successful_connections;
        } catch (const std::exception& ex) {
            RESTC_CPP_LOG_ERROR_("Future threw up: " << ex.what());
        }
    }

    RESTC_CPP_LOG_INFO_("We had " << successful_connections
        << " successful connections.");

    EXPECT_EQ(CONNECTIONS, successful_connections);

    mutex.unlock();

    rest_client->CloseWhenReady();
}


int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("info");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
