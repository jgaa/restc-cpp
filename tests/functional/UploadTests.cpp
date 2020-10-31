
#include <iostream>

// Include before boost::log headers
#include "restc-cpp/logging.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

#include <boost/filesystem.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/error.h"
#include "restc-cpp/RequestBuilder.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"


using namespace std;
using namespace restc_cpp;

boost::filesystem::path temp_path;

const lest::test specification[] = {

// The content is send un-encoded in the body
TEST(TestRawUpload)
{
    auto rest_client = RestClient::Create();
    rest_client->ProcessWithPromise([&](Context& ctx) {

        auto reply = RequestBuilder(ctx)
            .Post(GetDockerUrl("http://localhost:3001/upload_raw/"))
            .Header("Content-Type", "application/octet-stream")
            .File(temp_path)
            .Execute();

        CHECK_EQUAL(200, reply->GetResponseCode());

    }).get();
}


}; //lest

int main( int argc, char * argv[] )
{
    temp_path = boost::filesystem::unique_path();
    {
        ofstream file(temp_path.string());
        for(int i = 0; i < 1000; i++) {
            file << "This is line #" << i << endl;
        }
    }

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );
#endif

    const auto rval = lest::run( specification, argc, argv );

    boost::filesystem::remove(temp_path);

    return rval;
}

