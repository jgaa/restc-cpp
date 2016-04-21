
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

    size_t
    AsyncReadSome(boost::asio::mutable_buffers_1 read_buffers) override {

        if (next_buffer_ == test_buffers_.end())
            return 0;

        assert(boost::asio::buffer_size(read_buffers) >= next_buffer_->size());
        memcpy(boost::asio::buffer_cast<char *>(read_buffers),
               &(*next_buffer_)[0], next_buffer_->size());

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


TEST(TestFragmentedReply)
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

         CHECK_EQUAL("keep-alive", *reply.GetHeader("Connection"));
         CHECK_EQUAL("0", *reply.GetHeader("Content-Length"));

     }).wait();
}



int main(int, const char *[])
{
   return UnitTest::RunAllTests();
}

