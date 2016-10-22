
// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "../src/ReplyImpl.h"

#include "UnitTest++/UnitTest++.h"


using namespace std;
using namespace restc_cpp;

namespace restc_cpp{
namespace unittests {

    
class TestReply : public ReplyImpl
{
public:
    using test_buffers_t = std::list<std::string>;

    TestReply(Context& ctx, RestClient& owner, test_buffers_t& buffers)
    : ReplyImpl(nullptr, ctx, owner), test_buffers_{buffers}
    {
        next_buffer_ = test_buffers_.begin();
    }
    
//     string Dump(const char *p, const size_t len) {
//         std::string rval;
//         
//         for(size_t i = 0; i < len; i++) {
//             if (*p == '\r') {
//                 rval += "\\r";
//             } else if (*p == '\n') {
//                 rval += "\\n";
//             } else if (*p == 0) {
//                 rval += "\\0";
//             } else {
//                 rval += *p;
//             }
//             
//             ++p;
//         }
//         
//         return rval;
//     }

    size_t
    AsyncReadSome(boost::asio::mutable_buffers_1 read_buffers) override {

        if (next_buffer_ == test_buffers_.end())
            return 0;

        assert(boost::asio::buffer_size(read_buffers) >= next_buffer_->size());

        const boost::string_ref ret{*next_buffer_};

        memcpy(boost::asio::buffer_cast<char *>(read_buffers),
              ret.data(), ret.size());
        
        //cerr << "Inserting " << ret.size() << " bytes, " << " data: '" << Dump(ret.data(), ret.size()) << "'" << endl;
        

        auto rval = next_buffer_->size();
        ++next_buffer_;
        

        return rval;
    }

    void SimulateServerReply() {
        StartReceiveFromServer();
    }

private:
    test_buffers_t& test_buffers_;
    test_buffers_t::iterator next_buffer_;

};


} // unittests
} // restc_cpp


TEST(TestSimpleHeader)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "X-Powered-By: Express\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Cache-Control: no-cache\r\n"
        "Pragma: no-cache\r\n"
        "Expires: -1\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 0\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("keep-alive", *reply.GetHeader("Connection"));
         CHECK_EQUAL("Express", *reply.GetHeader("X-Powered-By"));
         CHECK_EQUAL("Origin, Accept-Encoding", *reply.GetHeader("Vary"));
         CHECK_EQUAL("no-cache", *reply.GetHeader("Cache-Control"));
         CHECK_EQUAL("no-cache", *reply.GetHeader("Pragma"));
         CHECK_EQUAL("-1", *reply.GetHeader("Expires"));
         CHECK_EQUAL("application/json; charset=utf-8", *reply.GetHeader("Content-Type"));
         CHECK_EQUAL("Thu, 21 Apr 2016 13:44:36 GMT", *reply.GetHeader("Date"));
         CHECK_EQUAL("0", *reply.GetHeader("Content-Length"));

     }).get();
}

TEST(TestSimpleSegmentedHeader)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n");
    buffer.push_back("Server: Cowboy\r\n");
    buffer.push_back("Connection: keep-alive\r\n");
    buffer.push_back("X-Powered-By: Express\r\n");
    buffer.push_back("Vary: Origin, Accept-Encoding\r\n");
    buffer.push_back("Cache-Control: no-cache\r\n");
    buffer.push_back("Pragma: no-cache\r\n");
    buffer.push_back("Expires: -1\r\n");
    buffer.push_back("Content-Type: application/json; charset=utf-8\r\n");
    buffer.push_back("Content-Length: 0\r\n");
    buffer.push_back("Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n");
    buffer.push_back("\r\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();

         CHECK_EQUAL("keep-alive", *reply.GetHeader("Connection"));
         CHECK_EQUAL("0", *reply.GetHeader("Content-Length"));

     }).get();
}

TEST(TestSimpleVerySegmentedHeader)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\nSer");
    buffer.push_back("ver: Cowboy\r\n");
    buffer.push_back("Connection: keep-alive\r");
    buffer.push_back("\nX-Powered-By: Express\r\nV");
    buffer.push_back("ary");
    buffer.push_back(": Origin, Accept-Encoding\r\nCache-Control: no-cache\r\n");
    buffer.push_back("Pragma: no-cache\r\n");
    buffer.push_back("Expires: -1\r\n");
    buffer.push_back("Content-Type: application/json; charset=utf-8\r\n");
    buffer.push_back("Content-Length: 0\r\n");
    buffer.push_back("Date: Thu, 21 Apr 2016 13:44:36 GMT");
    buffer.push_back("\r");
    buffer.push_back("\n");
    buffer.push_back("\r");
    buffer.push_back("\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();

         CHECK_EQUAL("keep-alive", *reply.GetHeader("Connection"));
         CHECK_EQUAL("0", *reply.GetHeader("Content-Length"));

     }).get();
}

TEST(TestSimpleBody)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 10\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n"
        "1234567890");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("10", *reply.GetHeader("Content-Length"));
         CHECK_EQUAL(10, (int)body.size());

     }).get();
}

TEST(TestSimpleBody2)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 10\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");
    buffer.push_back("1234567890");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("10", *reply.GetHeader("Content-Length"));
         CHECK_EQUAL(10, (int)body.size());

     }).get();
}

TEST(TestSimpleBody3)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 10\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");
    buffer.push_back("1234567");
    buffer.push_back("890");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("10", *reply.GetHeader("Content-Length"));
         CHECK_EQUAL(10, (int)body.size());

     }).get();
}

TEST(TestSimpleBody4)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: 10\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n12");
    buffer.push_back("34567");
    buffer.push_back("890");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("10", *reply.GetHeader("Content-Length"));
         CHECK_EQUAL(10, (int)body.size());

     }).get();
}

TEST(TestChunkedBody)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n"
        "4\r\nWiki\r\n5\r\npedia\r\nE\r\n in\r\n\r\nchunks."
        "\r\n0\r\n\r\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));
         CHECK_EQUAL((0x4 + 0x5 + 0xE), (int)body.size());

     }).get();
}

TEST(TestChunkedBody2)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");
    buffer.push_back("4\r\nWiki\r\n");
    buffer.push_back("5\r\npedia\r\n");
    buffer.push_back("E\r\n in\r\n\r\nchunks.\r\n");
    buffer.push_back("0\r\n\r\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));
         CHECK_EQUAL((0x4 + 0x5 + 0xE), (int)body.size());

     }).get();
}

TEST(TestChunkedBody4)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");
    buffer.push_back("4\r\nW");
    buffer.push_back("iki\r\n5\r\npedi");
    buffer.push_back("a\r\nE\r\n in\r\n\r\nchunks.\r");
    buffer.push_back("\n");
    buffer.push_back("0");
    buffer.push_back("\r");
    buffer.push_back("\n");
    buffer.push_back("\r");
    buffer.push_back("\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();
         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));
         CHECK_EQUAL((0x4 + 0x5 + 0xE), (int)body.size());

     }).get();
}



TEST(TestChunkedTrailer)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");
    buffer.push_back("4\r\nWiki\r\n");
    buffer.push_back("5\r\npedia\r\n");
    buffer.push_back("E\r\n in\r\n\r\nchunks.\r\n");
    buffer.push_back("0\r\n");
    buffer.push_back("Server: Indian\r\n");
    buffer.push_back("Connection: close\r\n");
    buffer.push_back("\r\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("keep-alive", *reply.GetHeader("Connection"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));

         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Indian", *reply.GetHeader("Server"));
         CHECK_EQUAL("close", *reply.GetHeader("Connection"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));
         CHECK_EQUAL((0x4 + 0x5 + 0xE), (int)body.size());

     }).get();
}

TEST(TestChunkedParameterAndTrailer)
{
    ::restc_cpp::unittests::TestReply::test_buffers_t buffer;

    buffer.push_back("HTTP/1.1 200 OK\r\n"
        "Server: Cowboy\r\n"
        "Connection: keep-alive\r\n"
        "Vary: Origin, Accept-Encoding\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
        "\r\n");
    buffer.push_back("4;test=1;tset=\"yyyy\"\r\nWiki\r\n");
    buffer.push_back("5;more-to-follow\r\npedia\r\n");
    buffer.push_back("E;77\r\n in\r\n\r\nchunks.\r\n");
    buffer.push_back("0;this-is-the-end\r\n");
    buffer.push_back("Server: Indian\r\n");
    buffer.push_back("Connection: close\r\n");
    buffer.push_back("\r\n");

     auto rest_client = RestClient::Create();
     rest_client->ProcessWithPromise([&](Context& ctx) {

         ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

         reply.SimulateServerReply();

         CHECK_EQUAL("Cowboy", *reply.GetHeader("Server"));
         CHECK_EQUAL("keep-alive", *reply.GetHeader("Connection"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));

         auto body = reply.GetBodyAsString();

         CHECK_EQUAL("Indian", *reply.GetHeader("Server"));
         CHECK_EQUAL("close", *reply.GetHeader("Connection"));
         CHECK_EQUAL("chunked", *reply.GetHeader("Transfer-Encoding"));
         CHECK_EQUAL((0x4 + 0x5 + 0xE), (int)body.size());

     }).get();
}

int main(int, const char *[])
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );

    return UnitTest::RunAllTests();
}

