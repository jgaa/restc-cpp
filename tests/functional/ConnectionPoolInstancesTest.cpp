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


STARTCASE(UseAfterDelete) {

    for(auto i = 0; i < 500; ++i) {

        RestClient::Create()->ProcessWithPromiseT<int>([&](Context& ctx) {
            auto repl = ctx.Get(GetDockerUrl(http_url));
            repl->GetBodyAsString();
            return 0;
        });

        RestClient::Create()->ProcessWithPromiseT<int>([&](Context& ctx) {
            auto repl = ctx.Get(GetDockerUrl(http_url));
            repl->GetBodyAsString();
            return 0;
        }).get();


        if ((i % 100) == 0) {
            clog << '#' << (i +1) << endl;
        }
    }

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
