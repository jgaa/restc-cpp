
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
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
    std::vector<Person> members;
    std::list<Person> more_members;
};

BOOST_FUSION_ADAPT_STRUCT(
    Group,
    (std::string, name)
    (int, gid)
    (Person, leader)
    (std::vector<Person>, members)
    (std::list<Person>, more_members)
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
    assert(boost::fusion::traits::is_sequence<Group>::value);
    assert(boost::fusion::traits::is_sequence<Person>::value);

    Group group;
    std::string json =
        R"({"name" : "qzar", "gid" : 1, "leader" : { "id" : 100, "name" : "Dolly Doe", "balance" : 123.45 },)"
        R"("members" : [{ "id" : 101, "name" : "m1", "balance" : 0.0}, { "id" : 102, "name" : "m2", "balance" : 1.0}],)"
        R"("more_members" : [{ "id" : 103, "name" : "m3", "balance" : 0.1}, { "id" : 104, "name" : "m4", "balance" : 2.0}])"
        R"(})";

    RapidJsonHandlerImpl<Group> handler(group);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(1, group.gid);
    CHECK_EQUAL("qzar", group.name);
    CHECK_EQUAL(100, group.leader.id);
    CHECK_EQUAL("Dolly Doe", group.leader.name);
    CHECK_EQUAL(123.45, group.leader.balance);
    CHECK_EQUAL(2, group.members.size());
    CHECK_EQUAL(101, group.members[0].id);
    CHECK_EQUAL("m1", group.members[0].name);
    CHECK_EQUAL(0.0, group.members[0].balance);
    CHECK_EQUAL(102, group.members[1].id);
    CHECK_EQUAL("m2", group.members[1].name);
    CHECK_EQUAL(1.0, group.members[1].balance);
    CHECK_EQUAL(2, group.more_members.size());
    CHECK_EQUAL(103, group.more_members.front().id);
    CHECK_EQUAL("m3", group.more_members.front().name);
    CHECK_EQUAL(0.1, group.more_members.front().balance);
    CHECK_EQUAL(104, group.more_members.back().id);
    CHECK_EQUAL("m4", group.more_members.back().name);
    CHECK_EQUAL(2.0, group.more_members.back().balance);
}

TEST(DeserializeIntVector)
{
    std::string json = R"([1,2,3,4,5,6,7,8,9,10])";

    std::vector<int> ints;
    RapidJsonHandlerImpl<decltype(ints)> handler(ints);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(10, ints.size());

    auto val = 0;
    for(auto v : ints) {
        CHECK_EQUAL(++val, v);
    }
}

int main(int, const char *[])
{
   return UnitTest::RunAllTests();
}

