
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/filesystem.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/error.h"
#include "restc-cpp/RequestBuilder.h"

#include "UnitTest++/UnitTest++.h"

using namespace std;
using namespace restc_cpp;

boost::filesystem::path temp_path;

// The content is send un-encoded in the body
TEST(TestRawUpload)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Post("http://localhost:3001/upload_raw/")
            .Header("Content-Type", "application/octet-stream")
            .File(temp_path)
            .Execute();

        CHECK_EQUAL(200, reply->GetResponseCode());

    }).get();
}


int main(int, const char *[])
{

    temp_path = boost::filesystem::unique_path();
    {
        ofstream file(temp_path.string());
        for(int i = 0; i < 1000; i++) {
            file << "This is line #" << i << endl;
        }
    }

    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );

    auto rval = UnitTest::RunAllTests();

    boost::filesystem::remove(temp_path);

    return rval;
}

