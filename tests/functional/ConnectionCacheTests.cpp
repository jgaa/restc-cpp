
// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/boost_compatibility.h"

#include "../src/ReplyImpl.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_url = "http://localhost:3001/normal/posts";
const string http_url_many = "http://localhost:3001/normal/manyposts";
const string http_connection_close_url = "http://localhost:3001/close/posts";

using namespace std;
using namespace restc_cpp;


TEST(ConnectionCache, ConnectionRecycling) {

    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl_one = ctx.Get(GetDockerUrl(http_url));
        auto first_conn_id = repl_one->GetConnectionId();
        EXPECT_EQ(200, repl_one->GetResponseCode());
        // Discard all data
        while(repl_one->MoreDataToRead()) {
            repl_one->GetSomeData();
        }

        auto repl_two = ctx.Get(GetDockerUrl(http_url));
        auto second_conn_id = repl_two->GetConnectionId();
        EXPECT_EQ(200, repl_two->GetResponseCode());
        // Discard all data
        while(repl_two->MoreDataToRead()) {
            repl_two->GetSomeData();
        }

        EXPECT_EQ(first_conn_id, second_conn_id);

    }).get();
}

// Test that we honor 'Connection: close' server header
TEST(ConnectionCache, ConnectionClose) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl_one = ctx.Get(GetDockerUrl(http_connection_close_url));

        EXPECT_EQ(200, repl_one->GetResponseCode());

        // Discard all data
        while(repl_one->MoreDataToRead()) {
            repl_one->GetSomeData();
        }

        EXPECT_EQ(0, static_cast<int>(
            rest_client->GetConnectionPool()->GetIdleConnections()));

    }).get();
}

TEST(ConnectionCache, MaxConnectionsToEndpoint) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    std::vector<Connection::ptr_t> connections;
    const auto ep = boost_create_endpoint("127.0.0.1", 80);
    for(size_t i = 0; i < config->cacheMaxConnectionsPerEndpoint; ++i) {
        connections.push_back(pool->GetConnection(ep, restc_cpp::Connection::Type::HTTP));
    }

    EXPECT_THROW(pool->GetConnection(ep,
            restc_cpp::Connection::Type::HTTP), std::runtime_error);
}

TEST(ConnectionCache, MaxConnections) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    auto addr = boost_convert_ipv4_to_uint("127.0.0.1");

    std::vector<Connection::ptr_t> connections;
    decltype(addr) i = 0;
    for(; i < config->cacheMaxConnections; ++i) {
        connections.push_back(pool->GetConnection(
            boost::asio::ip::tcp::endpoint{
                    boost::asio::ip::address_v4{static_cast<unsigned int >(addr + i)}, 80},
            restc_cpp::Connection::Type::HTTP));
    }

    EXPECT_THROW(pool->GetConnection(
            boost::asio::ip::tcp::endpoint{
                    boost::asio::ip::address_v4{static_cast<unsigned int>(addr + i)}, 80},
            restc_cpp::Connection::Type::HTTP), std::runtime_error);
}

TEST(ConnectionCache, CleanupTimer) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    config->cacheTtlSeconds = 2;
    config->cacheCleanupIntervalSeconds = 1;

    rest_client->ProcessWithPromise([&](Context& ctx) {
        auto repl = ctx.Get(GetDockerUrl(http_url));
        EXPECT_EQ(200, repl->GetResponseCode());

        // Discard all data
        while(repl->MoreDataToRead()) {
            repl->GetSomeData();
        }

    }).get();

    EXPECT_EQ(1, static_cast<int>(pool->GetIdleConnections()));

    std::this_thread::sleep_for(std::chrono::seconds(4));

    EXPECT_EQ(0, static_cast<int>(pool->GetIdleConnections()));
}

TEST(ConnectionCache, PrematureCloseNotRecycled) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl_one = ctx.Get(GetDockerUrl(http_url_many));

        EXPECT_EQ(200, repl_one->GetResponseCode());

        repl_one.reset();

        EXPECT_EQ(0, static_cast<int>(
            rest_client->GetConnectionPool()->GetIdleConnections()));

    }).get();
}

TEST(ConnectionCache, OverrideMaxConnectionsToEndpoint) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    std::vector<Connection::ptr_t> connections;
    auto const ep  = boost_create_endpoint("127.0.0.1", 80);
    for(size_t i = 0; i < config->cacheMaxConnectionsPerEndpoint; ++i) {
        connections.push_back(pool->GetConnection(ep, restc_cpp::Connection::Type::HTTP));
    }

    connections.push_back(pool->GetConnection(ep, restc_cpp::Connection::Type::HTTP, true));
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
