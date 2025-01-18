
// Include before boost::log headers
#include "restc-cpp/logging.h"

#include "../src/ReplyImpl.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;

namespace restc_cpp::unittests {

using test_buffers_t = std::list<std::string>;

class MockReader : public DataReader {
public:
    MockReader(test_buffers_t& buffers)
    : test_buffers_{buffers} {

        next_buffer_ = test_buffers_.begin();
    }

    void Finish() override {
    }

    [[nodiscard]] bool IsEof() const override { return next_buffer_ == test_buffers_.end(); }

    ::restc_cpp::boost_const_buffer ReadSome() override {
        if (IsEof()) {
            return {nullptr, 0};
        }

        size_t const data_len = next_buffer_->size();
        const char * const data = next_buffer_->c_str();
        ++next_buffer_;
        return {data, data_len};
    }

    test_buffers_t& test_buffers_;
    test_buffers_t::iterator next_buffer_;
};

class TestReply : public ReplyImpl
{
public:
    TestReply(Context& ctx, RestClient& owner, test_buffers_t& buffers)
    : ReplyImpl(nullptr, ctx, owner, Request::Type::GET), buffers_{buffers}
    {
    }

    void SimulateServerReply() {
        StartReceiveFromServer(make_unique<MockReader>(buffers_));
    }

private:
    test_buffers_t& buffers_;
};

} // namespace restc_cpp::unittests
// restc_cpp

TEST(HttpReply, SimpleHeader)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
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
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("keep-alive", *reply.GetHeader("Connection"));
        EXPECT_EQ("Express", *reply.GetHeader("X-Powered-By"));
        EXPECT_EQ("Origin, Accept-Encoding", *reply.GetHeader("Vary"));
        EXPECT_EQ("no-cache", *reply.GetHeader("Cache-Control"));
        EXPECT_EQ("no-cache", *reply.GetHeader("Pragma"));
        EXPECT_EQ("-1", *reply.GetHeader("Expires"));
        EXPECT_EQ("application/json; charset=utf-8", *reply.GetHeader("Content-Type"));
        EXPECT_EQ("Thu, 21 Apr 2016 13:44:36 GMT", *reply.GetHeader("Date"));
        EXPECT_EQ("0", *reply.GetHeader("Content-Length"));
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, SimpleSegmentedHeader)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n");
    buffer.emplace_back("Server: Cowboy\r\n");
    buffer.emplace_back("Connection: keep-alive\r\n");
    buffer.emplace_back("X-Powered-By: Express\r\n");
    buffer.emplace_back("Vary: Origin, Accept-Encoding\r\n");
    buffer.emplace_back("Cache-Control: no-cache\r\n");
    buffer.emplace_back("Pragma: no-cache\r\n");
    buffer.emplace_back("Expires: -1\r\n");
    buffer.emplace_back("Content-Type: application/json; charset=utf-8\r\n");
    buffer.emplace_back("Content-Length: 0\r\n");
    buffer.emplace_back("Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n");
    buffer.emplace_back("\r\n");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();

        EXPECT_EQ("keep-alive", *reply.GetHeader("Connection"));
        EXPECT_EQ("0", *reply.GetHeader("Content-Length"));
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, SimpleVerySegmentedHeader)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\nSer");
    buffer.emplace_back("ver: Cowboy\r\n");
    buffer.emplace_back("Connection: keep-alive\r");
    buffer.emplace_back("\nX-Powered-By: Express\r\nV");
    buffer.emplace_back("ary");
    buffer.emplace_back(": Origin, Accept-Encoding\r\nCache-Control: no-cache\r\n");
    buffer.emplace_back("Pragma: no-cache\r\n");
    buffer.emplace_back("Expires: -1\r\n");
    buffer.emplace_back("Content-Type: application/json; charset=utf-8\r\n");
    buffer.emplace_back("Content-Length: 0\r\n");
    buffer.emplace_back("Date: Thu, 21 Apr 2016 13:44:36 GMT");
    buffer.emplace_back("\r");
    buffer.emplace_back("\n");
    buffer.emplace_back("\r");
    buffer.emplace_back("\n");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();

        EXPECT_EQ("keep-alive", *reply.GetHeader("Connection"));
        EXPECT_EQ("0", *reply.GetHeader("Content-Length"));
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, SimpleBody)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Content-Length: 10\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n"
                        "1234567890");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("10", *reply.GetHeader("Content-Length"));
        EXPECT_EQ(10, (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, SimpleBody2)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Content-Length: 10\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n");
    buffer.emplace_back("1234567890");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("10", *reply.GetHeader("Content-Length"));
        EXPECT_EQ(10, (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, SimpleBody3)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Content-Length: 10\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n");
    buffer.emplace_back("1234567");
    buffer.emplace_back("890");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("10", *reply.GetHeader("Content-Length"));
        EXPECT_EQ(10, (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, SimpleBody4)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Content-Length: 10\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n12");
    buffer.emplace_back("34567");
    buffer.emplace_back("890");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("10", *reply.GetHeader("Content-Length"));
        EXPECT_EQ(10, (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, ChunkedBody)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
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
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));
        EXPECT_EQ((0x4 + 0x5 + 0xE), (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, ChunkedBody2)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n");
    buffer.emplace_back("4\r\nWiki\r\n");
    buffer.emplace_back("5\r\npedia\r\n");
    buffer.emplace_back("E\r\n in\r\n\r\nchunks.\r\n");
    buffer.emplace_back("0\r\n\r\n");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));
        EXPECT_EQ((0x4 + 0x5 + 0xE), (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, ChunkedBody4)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n");
    buffer.emplace_back("4\r\nW");
    buffer.emplace_back("iki\r\n5\r\npedi");
    buffer.emplace_back("a\r\nE\r\n in\r\n\r\nchunks.\r");
    buffer.emplace_back("\n");
    buffer.emplace_back("0");
    buffer.emplace_back("\r");
    buffer.emplace_back("\n");
    buffer.emplace_back("\r");
    buffer.emplace_back("\n");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();
        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));
        EXPECT_EQ((0x4 + 0x5 + 0xE), (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, ChunkedTrailer)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n");
    buffer.emplace_back("4\r\nWiki\r\n");
    buffer.emplace_back("5\r\npedia\r\n");
    buffer.emplace_back("E\r\n in\r\n\r\nchunks.\r\n");
    buffer.emplace_back("0\r\n");
    buffer.emplace_back("Server: Indian\r\n");
    buffer.emplace_back("Connection: close\r\n");
    buffer.emplace_back("\r\n");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("keep-alive", *reply.GetHeader("Connection"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));

        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Indian", *reply.GetHeader("Server"));
        EXPECT_EQ("close", *reply.GetHeader("Connection"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));
        EXPECT_EQ((0x4 + 0x5 + 0xE), (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

TEST(HttpReply, ChunkedParameterAndTrailer)
{
    ::restc_cpp::unittests::test_buffers_t buffer;

    buffer.emplace_back("HTTP/1.1 200 OK\r\n"
                        "Server: Cowboy\r\n"
                        "Connection: keep-alive\r\n"
                        "Vary: Origin, Accept-Encoding\r\n"
                        "Content-Type: application/json; charset=utf-8\r\n"
                        "Transfer-Encoding: chunked\r\n"
                        "Date: Thu, 21 Apr 2016 13:44:36 GMT\r\n"
                        "\r\n");
    buffer.emplace_back("4;test=1;tset=\"yyyy\"\r\nWiki\r\n");
    buffer.emplace_back("5;more-to-follow\r\npedia\r\n");
    buffer.emplace_back("E;77\r\n in\r\n\r\nchunks.\r\n");
    buffer.emplace_back("0;this-is-the-end\r\n");
    buffer.emplace_back("Server: Indian\r\n");
    buffer.emplace_back("Connection: close\r\n");
    buffer.emplace_back("\r\n");

    auto rest_client = RestClient::Create();
    auto f = rest_client->ProcessWithPromise([&](Context &ctx) {
        ::restc_cpp::unittests::TestReply reply(ctx, *rest_client, buffer);

        reply.SimulateServerReply();

        EXPECT_EQ("Cowboy", *reply.GetHeader("Server"));
        EXPECT_EQ("keep-alive", *reply.GetHeader("Connection"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));

        auto body = reply.GetBodyAsString();

        EXPECT_EQ("Indian", *reply.GetHeader("Server"));
        EXPECT_EQ("close", *reply.GetHeader("Connection"));
        EXPECT_EQ("chunked", *reply.GetHeader("Transfer-Encoding"));
        EXPECT_EQ((0x4 + 0x5 + 0xE), (int) body.size());
    });

    EXPECT_NO_THROW(f.get());
}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
