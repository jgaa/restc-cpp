


// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBuilder.h"
#include "restc-cpp/SerializeJson.h"
#include "restc-cpp/IteratorFromJsonSerializer.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "UnitTest++/UnitTest++.h"

using namespace std;
using namespace restc_cpp;

#define CONNECTIONS 20

struct Post {
    int id = 0;
    string username;
    string motto;
};

BOOST_FUSION_ADAPT_STRUCT(
    Post,
    (int, id)
    (string, username)
    (string, motto)
)

namespace restc_cpp{
namespace unittests {

const string http_url = "http://localhost:3000/manyposts";

TEST(TestOwnIoservice)
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

            auto reply = RequestBuilder(ctx)
                .Get(http_url)
                .Execute();

            // Use an iterator to make it simple to fetch some data and
            // then wait on the mutex before we finish.
            IteratorFromJsonSerializer<Post> results(*reply);

            auto it = results.begin();
            RESTC_CPP_LOG_DEBUG << "Iteration #" << i
                << " Read item # " << it->id;

            // We can't just wait on the lock since we are in a co-routine.
            // So we use the async_wait() to poll in stead.
            while(!mutex.try_lock()) {
                boost::asio::deadline_timer timer(rest_client->GetIoService(),
                    boost::posix_time::milliseconds(1));
                timer.async_wait(ctx.GetYield());
            }
            mutex.unlock();

            // Fetch the rest
            for(; it != results.end(); ++it)
                ;

            promises[i].set_value(i);
        });
    }

    thread worker([&ioservice]() {
        cout << "ioservice is running" << endl;
        ioservice.run();
        cout << "ioservice is done" << endl;
    });

    mutex.unlock();

    int successful_connections = 0;
    for(auto& future : futures) {
        try {
            auto i = future.get();
            RESTC_CPP_LOG_DEBUG << "Iteration #" << i << " is done";
            ++successful_connections;
        } catch (const std::exception& ex) {
            RESTC_CPP_LOG_ERROR << "Future threw up: " << ex.what();
        }
    }

    RESTC_CPP_LOG_INFO << "We had " << successful_connections
        << " successful connections.";

    CHECK_EQUAL(CONNECTIONS, successful_connections);

    rest_client->CloseWhenReady();
    ioservice.stop();
    worker.join();
}


}} // namespaces

int main(int, const char *[])
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );

    return UnitTest::RunAllTests();
}

