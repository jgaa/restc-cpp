
#include <cstdio>

#include "restc-cpp/logging.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"


using namespace std;
using namespace restc_cpp;
using namespace rapidjson;

struct Config {
    int max_something = {};
    string name;
    string url;
};

BOOST_FUSION_ADAPT_STRUCT(
    Config,
    (int, max_something)
    (string, name)
    (string, url)
)

const lest::test specification[] = {

STARTCASE(ReadConfigurationFromFile) {
    auto tmpname = boost::filesystem::unique_path();
    BOOST_SCOPE_EXIT(&tmpname) {
        boost::filesystem::remove(tmpname);
    } BOOST_SCOPE_EXIT_END

    {
        ofstream json_out(tmpname.native());
        json_out << '{' << endl
            << R"("max_something":100,)" << endl
            << R"("name":"Test Data",)" << endl
            << R"("url":"https://www.example.com")" << endl
            << '}';
    }

    ifstream ifs(tmpname.native());
    Config config;
    SerializeFromJson(config, ifs);

    EXPECT(config.max_something == 100);
    EXPECT(config.name == "Test Data");
    EXPECT(config.url == "https://www.example.com");
} ENDCASE

}; // lest


int main( int argc, char * argv[] )
{
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );
    return lest::run( specification, argc, argv );
}
