
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"
#include <boost/filesystem.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/error.h"
#include "restc-cpp/RequestBuilder.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

using namespace std;
using namespace restc_cpp;

boost::filesystem::path temp_path;

// The content is send un-encoded in the body
TEST(Upload, Raw)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Post(GetDockerUrl("http://localhost:3001/upload_raw/"))
            .Header("Content-Type", "application/octet-stream")
            .File(temp_path)
            .Execute();

        EXPECT_EQ(200, reply->GetResponseCode());

    }).get();
}


int main( int argc, char * argv[] )
{
    temp_path = boost::filesystem::unique_path();
    {
        ofstream file(temp_path.string());
        for(int i = 0; i < 1000; i++) {
            file << "This is line #" << i << endl;
        }
    }

    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    auto rval = RUN_ALL_TESTS();

    boost::filesystem::remove(temp_path);

    return rval;
}

