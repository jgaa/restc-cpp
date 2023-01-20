
#include <cstdio>
#include <sstream>

#include "restc-cpp/logging.h"
#include <boost/fusion/adapted.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "gtest/gtest.h"
#include "restc-cpp/test_helper.h"

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


TEST(IOstream2Json, ReadConfigurationFromFile) {
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

    EXPECT_TRUE(config.max_something == 100);
    EXPECT_TRUE(config.name == "Test Data");
    EXPECT_TRUE(config.url == "https://www.example.com");
}

TEST(IOstream2Json, WriteJsonToStream) {

    stringstream out;
    Config config;
    config.max_something = 100;
    config.name = "John";
    config.url = "https://www.example.com";

    SerializeToJson(config, out);

    EXPECT_EQ(out.str(), R"({"max_something":100,"name":"John","url":"https://www.example.com"})");
}

TEST(IOstream2Json, issue58) {

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

}

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();;
}
