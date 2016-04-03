
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Serialize.h"
#include "restc-cpp/SerializeJson.h"

#include "UnitTest++/UnitTest++.h"

using namespace std;
using namespace restc_cpp;
using namespace rapidjson;

struct Person {
    int id = 0;
    std::string name;
    double balance = 0;
};

BOOST_FUSION_ADAPT_STRUCT(
    Person,
    (int, id)
    (std::string, name)
    (double, balance)
)

struct Group {

    std::string name;
    int gid = 0;

    Person leader;
};

BOOST_FUSION_ADAPT_STRUCT(
    Group,
    (std::string, name)
    (int, gid)
    (Person, leader)
)

TEST(DeserializeSimpleObject)
{
    Person person;
    std::string json = R"({ "id" : 100, "name" : "John Doe", "balance" : 123.45 })";

    RapidJsonHandlerImpl<Person> handler(person);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(person.id, 100);
    CHECK_EQUAL(person.name, "John Doe");
    CHECK_EQUAL(person.balance, 123.45);
}

TEST(DeserializeNestedObject)
{
    Group group;
    std::string json = R"({"name" : "qzar", "gid" : 1, "leader" : { "id" : 100, "name" : "John Doe", "balance" : 123.45 }})";

    RapidJsonHandlerImpl<Group> handler(group);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(group.gid, 1);
    CHECK_EQUAL(group.name, "qzar");
    CHECK_EQUAL(group.leader.id, 100);
    CHECK_EQUAL(group.leader.name, "John Doe");
    CHECK_EQUAL(group.leader.balance, 123.45);
}

int main(int, const char *[])
{
   return UnitTest::RunAllTests();
}

