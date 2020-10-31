
#include <cstdio>
#include <sstream>

#include "restc-cpp/logging.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#endif

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



// ---------------------- # 58

typedef vector<unsigned char> LOCAL;
typedef vector<unsigned char> GLOBAL;
typedef vector<unsigned char> ADDRESS;
struct 	MAC
{
    ADDRESS address;
};
BOOST_FUSION_ADAPT_STRUCT(
    MAC,
    (ADDRESS, address)
)
typedef vector<MAC> MACLIST;

struct DeviceList{
    LOCAL local;
    GLOBAL global;
    MACLIST maclst;
};
BOOST_FUSION_ADAPT_STRUCT(
    DeviceList,
    (LOCAL, local)
    (GLOBAL, global)
    (MACLIST, maclst)
)
typedef vector<DeviceList> DeviceLst;
struct Config2 {
    int nIdSchedule = {};
    int nDCUNo;
    DeviceLst lst;
};
BOOST_FUSION_ADAPT_STRUCT(
    Config2,
    (int, nIdSchedule)
    (int, nDCUNo)
    (DeviceLst, lst)
)
/////////////////////////////////



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

STARTCASE(WriteJsonToStream) {

    stringstream out;
    Config config;
    config.max_something = 100;
    config.name = "John";
    config.url = "https://www.example.com";

    SerializeToJson(config, out);

    EXPECT(out.str() == R"({"max_something":100,"name":"John","url":"https://www.example.com"})");

} ENDCASE

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

STARTCASE(issue58) {

    auto tmpname = boost::filesystem::unique_path();
    BOOST_SCOPE_EXIT(&tmpname) {
        boost::filesystem::remove(tmpname);
    } BOOST_SCOPE_EXIT_END

    {
        ofstream json_out(tmpname.native());
        json_out << R"({"nIdSchedule":5,"nDCUNo":104400,"lst":[{"local":[65,66,67,68,69,69,70,80],"global":[71,72,73,74,75,76,77,78],"maclst":[{"address":[48,49,65,73,74,75,76,78]}]}]})";
    }

    Config2 config;
    ifstream ifs(tmpname.c_str());
    if (ifs.is_open())
    {
        // Read the ;config file into the config object.
        SerializeFromJson(config, ifs);
        cout<<"done"<<endl;
    }
    ofstream ofs(tmpname.c_str());
    config.lst[0].maclst[0].address[2] = 11;
    config.lst[0].maclst[0].address[3] = 11;
    config.lst[0].maclst[0].address[4] = 11;
    SerializeToJson(config, ofs);
    cout<<"done"<<endl;

} ENDCASE

}; // lest


int main( int argc, char * argv[] )
{
#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::trace
    );
#endif

    return lest::run( specification, argc, argv );
}
