// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;

TEST(RequestBuilder, DataWithCString)
{
    RequestBuilder rb;
    rb.Data("{}");
    EXPECT_EQ("{}", rb.GetData());

}

TEST(RequestBuilder, DataWithCStringLen)
{
    RequestBuilder rb;
    rb.Data("{}12345", 2);
    EXPECT_EQ("{}", rb.GetData());

}

TEST(RequestBuilder, DataWithString)
{

    const string data{"{}"};

    RequestBuilder rb;
    rb.Data(data);
    EXPECT_EQ(data, rb.GetData());

}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
