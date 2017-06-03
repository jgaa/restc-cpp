
// Include before boost::log headers
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "../src/ReplyImpl.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

/* These url's points to a local Docker container with nginx, linked to
 * a jsonserver docker container with mock data.
 * The scripts to build and run these containers are in the ./tests directory.
 */
const string http_url = "http://localhost:3001/normal/posts";
const string http_url_many = "http://localhost:3001/normal/manyposts";
const string http_connection_close_url = "http://localhost:3001/close/posts";

using namespace std;
using namespace restc_cpp;

const lest::test specification[] = {

STARTCASE(TestConnectionRecycling) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl_one = ctx.Get(GetDockerUrl(http_url));
        auto first_conn_id = repl_one->GetConnectionId();
        CHECK_EQUAL(200, repl_one->GetResponseCode());
        // Discard all data
        while(repl_one->MoreDataToRead()) {
            repl_one->GetSomeData();
        }

        auto repl_two = ctx.Get(GetDockerUrl(http_url));
        auto second_conn_id = repl_two->GetConnectionId();
        CHECK_EQUAL(200, repl_two->GetResponseCode());
        // Discard all data
        while(repl_two->MoreDataToRead()) {
            repl_two->GetSomeData();
        }

        CHECK_EQUAL(first_conn_id, second_conn_id);

    }).get();
} ENDCASE

// Test that we honor 'Connection: close' server header
STARTCASE(TestConnectionClose) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl_one = ctx.Get(GetDockerUrl(http_connection_close_url));

        CHECK_EQUAL(200, repl_one->GetResponseCode());

        // Discard all data
        while(repl_one->MoreDataToRead()) {
            repl_one->GetSomeData();
        }

        CHECK_EQUAL(0, static_cast<int>(
            rest_client->GetConnectionPool()->GetIdleConnections().get()));

    }).get();
} ENDCASE

STARTCASE(TestMaxConnectionsToEndpoint) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    std::vector<Connection::ptr_t> connections;
    boost::asio::ip::tcp::endpoint ep{
        boost::asio::ip::address::from_string("127.0.0.1"), 80};
    for(size_t i = 0; i < config->cacheMaxConnectionsPerEndpoint; ++i) {
        connections.push_back(pool->GetConnection(ep, restc_cpp::Connection::Type::HTTP));
    }

    EXPECT_THROWS_AS(pool->GetConnection(ep,
            restc_cpp::Connection::Type::HTTP), std::runtime_error);
} ENDCASE

STARTCASE(TestMaxConnections) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    auto addr = boost::asio::ip::address_v4::from_string("127.0.0.1").to_ulong();

    std::vector<Connection::ptr_t> connections;
    decltype(addr) i = 0;
    for(; i < config->cacheMaxConnections; ++i) {
        connections.push_back(pool->GetConnection(
            boost::asio::ip::tcp::endpoint{
                    boost::asio::ip::address_v4{addr + i}, 80},
            restc_cpp::Connection::Type::HTTP));
    }

    EXPECT_THROWS_AS(pool->GetConnection(
            boost::asio::ip::tcp::endpoint{
                    boost::asio::ip::address_v4{addr + i}, 80},
            restc_cpp::Connection::Type::HTTP), std::runtime_error);
} ENDCASE

STARTCASE(TestCleanupTimer) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    config->cacheTtlSeconds = 2;
    config->cacheCleanupIntervalSeconds = 1;

    rest_client->ProcessWithPromise([&](Context& ctx) {
        auto repl = ctx.Get(GetDockerUrl(http_url));
        CHECK_EQUAL(200, repl->GetResponseCode());

        // Discard all data
        while(repl->MoreDataToRead()) {
            repl->GetSomeData();
        }

    }).get();

    CHECK_EQUAL(1, static_cast<int>(pool->GetIdleConnections().get()));

    std::this_thread::sleep_for(std::chrono::seconds(4));

    CHECK_EQUAL(0, static_cast<int>(pool->GetIdleConnections().get()));
} ENDCASE

STARTCASE(TestPrematureCloseNotRecycled) {
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto repl_one = ctx.Get(GetDockerUrl(http_url_many));

        CHECK_EQUAL(200, repl_one->GetResponseCode());

        repl_one.reset();

        CHECK_EQUAL(0, static_cast<int>(
            rest_client->GetConnectionPool()->GetIdleConnections().get()));

    }).get();
} ENDCASE

STARTCASE(TestOverrideMaxConnectionsToEndpoint) {
    auto rest_client = RestClient::Create();
    auto pool = rest_client->GetConnectionPool();
    auto config = rest_client->GetConnectionProperties();

    std::vector<Connection::ptr_t> connections;
    boost::asio::ip::tcp::endpoint ep{
        boost::asio::ip::address::from_string("127.0.0.1"), 80};
    for(size_t i = 0; i < config->cacheMaxConnectionsPerEndpoint; ++i) {
        connections.push_back(pool->GetConnection(ep, restc_cpp::Connection::Type::HTTP));
    }

    connections.push_back(pool->GetConnection(ep, restc_cpp::Connection::Type::HTTP, true));
} ENDCASE

}; //lest

int main( int argc, char * argv[] )
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );
    return lest::run( specification, argc, argv );
}
