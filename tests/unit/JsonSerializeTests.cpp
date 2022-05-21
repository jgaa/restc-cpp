
#include <map>

#if (__cplusplus >= 201703L)
#   include <optional>
#endif


// Include before boost::log headers
#include "restc-cpp/logging.h"
#include <boost/fusion/adapted.hpp>

#include "restc-cpp/restc-cpp.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "restc-cpp/test_helper.h"
#include "lest/lest.hpp"

struct Data {
    int d = 123;
};

struct NoData {
    int d = 321;
};


namespace std {
    string to_string(const Data& d) {
        return to_string(d.d);
    }
}

#include "restc-cpp/SerializeJson.h"

using namespace std;
using namespace restc_cpp;
using namespace rapidjson;

struct Person {

    Person(int id_, std::string name_, double balance_)
    : id{id_}, name{std::move(name_)}, balance{balance_}
    {}

    Person& operator = (const Person&) = default;
    Person& operator = (Person&&) = default;
    Person() = default;
    Person(const Person&) = default;
    Person(Person&&) = default;

    int id = 0;
    std::string name;
    double balance = 0;
};

#if (__cplusplus >= 201703L)

struct Number {
    std::optional<int> value;
};

BOOST_FUSION_ADAPT_STRUCT(Number,
    (std::optional<int>, value)
);

struct Bool {
    bool value = false;
};

BOOST_FUSION_ADAPT_STRUCT(Bool,
    (bool, value)
);

struct Int {
    int value = 0;
};

BOOST_FUSION_ADAPT_STRUCT(Int,
    (int, value)
);

struct Numbers {
    int intval = 0;
    size_t sizetval = 0;
    uint32_t uint32 = 0;
    int64_t int64val = 0;
    uint64_t uint64val = 0;
};

BOOST_FUSION_ADAPT_STRUCT(Numbers,
    (int, intval)
    (size_t, sizetval)
    (uint32_t, uint32)
    (int64_t, int64val)
    (uint64_t, uint64val)
);

struct String {
    std::optional<string> value;
};

BOOST_FUSION_ADAPT_STRUCT(String,
    (std::optional<std::string>, value)
);

struct Pet {
    std::string name;
    std::string kind;
    std::optional<string> friends;
};

BOOST_FUSION_ADAPT_STRUCT(
    Pet,
    (std::string, name)
    (std::string, kind)
    (std::optional<string>, friends)
)

struct House {
    std::optional<bool> is_enabled;
    std::optional<Person> person;
    std::optional<Pet> pet;
};

BOOST_FUSION_ADAPT_STRUCT(
    House,
    (std::optional<bool>, is_enabled)
    (std::optional<Person>, person)
    (std::optional<Pet>, pet)
)

#endif

BOOST_FUSION_ADAPT_STRUCT(
    Person,
    (int, id)
    (std::string, name)
    (double, balance)
)

struct Quotes {
    int id;
    string origin;
    string quote;
};

BOOST_FUSION_ADAPT_STRUCT(
    Quotes,
    (int, id)
    (std::string, origin)
    (std::string, quote)
)

struct DataHolder {
    Data data;
};

BOOST_FUSION_ADAPT_STRUCT(
    DataHolder,
    (Data, data)
)

struct NoDataHolder {
    NoData data;
};

BOOST_FUSION_ADAPT_STRUCT(
    NoDataHolder,
    (NoData, data)
)


struct Group {

    Group(std::string name_, int gid_, Person leader_,
	  std::vector<Person> members_ = {},
          std::list<Person> more_members_ = {},
          std::deque<Person> even_more_members_ = {})
    : name{std::move(name_)}, gid{gid_}, leader{std::move(leader_)}
    , members{move(members_)}, more_members{move(more_members_)}
    , even_more_members{move(even_more_members_)}
    {}

    Group() = default;
    Group(const Group&) = default;
    Group(Group&&) = default;

    std::string name;
    int gid = 0;

    Person leader;
    std::vector<Person> members;
    std::list<Person> more_members;
    std::deque<Person> even_more_members;
};

BOOST_FUSION_ADAPT_STRUCT(
    Group,
    (std::string, name)
    (int, gid)
    (Person, leader)
    (std::vector<Person>, members)
    (std::list<Person>, more_members)
    (std::deque<Person>, even_more_members)
)

const lest::test specification[] = {

STARTCASE(SerializeSimpleObject) {
    Person person = { 100, "John Doe"s, 123.45 };

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(person), decltype(writer)>
        serializer(person, writer);

    serializer.Serialize();

    CHECK_EQUAL(R"({"id":100,"name":"John Doe","balance":123.45})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeNestedObject) {
    Group group = Group(string("Group name"), 99, Person( 100, string("John Doe"), 123.45 ));

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(group), decltype(writer)>
        serializer(group, writer);

    serializer.IgnoreEmptyMembers(false);
    serializer.Serialize();

    CHECK_EQUAL(R"({"name":"Group name","gid":99,"leader":{"id":100,"name":"John Doe","balance":123.45},"members":[],"more_members":[],"even_more_members":[]})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeVector)
{
    std::vector<int> ints = {-1,2,3,4,5,6,7,8,9,-10};

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(ints), decltype(writer)>
        serializer(ints, writer);

    serializer.Serialize();

    CHECK_EQUAL(R"([-1,2,3,4,5,6,7,8,9,-10])"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeList) {
    std::list<unsigned int> ints = {1,2,3,4,5,6,7,8,9,10};

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(ints), decltype(writer)>
        serializer(ints, writer);

    serializer.Serialize();

    CHECK_EQUAL(R"([1,2,3,4,5,6,7,8,9,10])"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeNestedVector)
{
    std::vector<std::vector<int>> nested_ints = {{-1,2,3},{4,5,-6}};

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(nested_ints), decltype(writer)>
        serializer(nested_ints, writer);

    serializer.Serialize();

    CHECK_EQUAL(R"([[-1,2,3],[4,5,-6]])"s,
                s.GetString());

} ENDCASE

STARTCASE(DeserializeSimpleObject) {
    Person person;
    std::string json = R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45 })";

    RapidJsonDeserializer<Person> handler(person);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(person.id, 100);
    CHECK_EQUAL(person.name, "John Longdue Doe");
    CHECK_EQUAL(person.balance, 123.45);
} ENDCASE

STARTCASE(DeserializeNestedObject) {
    assert(boost::fusion::traits::is_sequence<Group>::value);
    assert(boost::fusion::traits::is_sequence<Person>::value);

    Group group;
    std::string json =
        R"({"name" : "qzar", "gid" : 1, "leader" : { "id" : 100, "name" : "Dolly Doe", "balance" : 123.45 },)"
        R"("members" : [{ "id" : 101, "name" : "m1", "balance" : 0.0}, { "id" : 102, "name" : "m2", "balance" : 1.0}],)"
        R"("more_members" : [{ "id" : 103, "name" : "m3", "balance" : 0.1}, { "id" : 104, "name" : "m4", "balance" : 2.0}],)"
        R"("even_more_members" : [{ "id" : 321, "name" : "m10", "balance" : 0.1}, { "id" : 322, "name" : "m11", "balance" : 22.0}])"
        R"(})";

    RapidJsonDeserializer<Group> handler(group);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(1, group.gid);
    CHECK_EQUAL("qzar"s, group.name);
    CHECK_EQUAL(100, group.leader.id);
    CHECK_EQUAL("Dolly Doe"s, group.leader.name);
    CHECK_EQUAL(123.45, group.leader.balance);
    CHECK_EQUAL(2, static_cast<int>(group.members.size()));
    CHECK_EQUAL(101, group.members[0].id);
    CHECK_EQUAL("m1"s, group.members[0].name);
    CHECK_EQUAL(0.0, group.members[0].balance);
    CHECK_EQUAL(102, group.members[1].id);
    CHECK_EQUAL("m2"s, group.members[1].name);
    CHECK_EQUAL(1.0, group.members[1].balance);
    CHECK_EQUAL(2, static_cast<int>(group.more_members.size()));
    CHECK_EQUAL(103, group.more_members.front().id);
    CHECK_EQUAL("m3"s, group.more_members.front().name);
    CHECK_EQUAL(0.1, group.more_members.front().balance);
    CHECK_EQUAL(104, group.more_members.back().id);
    CHECK_EQUAL("m4"s, group.more_members.back().name);
    CHECK_EQUAL(2.0, group.more_members.back().balance);
    CHECK_EQUAL(321, group.even_more_members.front().id);
    CHECK_EQUAL("m10"s, group.even_more_members.front().name);
    CHECK_EQUAL(0.1, group.even_more_members.front().balance);
    CHECK_EQUAL(322, group.even_more_members.back().id);
    CHECK_EQUAL("m11"s, group.even_more_members.back().name);
    CHECK_EQUAL(22.0, group.even_more_members.back().balance);
} ENDCASE

STARTCASE(DeserializeIntVector) {
    std::string json = R"([1,2,3,4,5,6,7,8,9,10])";

    std::vector<int> ints;
    RapidJsonDeserializer<decltype(ints)> handler(ints);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(10, static_cast<int>(ints.size()));

    auto val = 0;
    for(auto v : ints) {
        CHECK_EQUAL(++val, v);
    }
} ENDCASE

STARTCASE(DeserializeNestedArray) {
    std::string json = R"([[1, 2, 3],[4, 5, 6]])";

    std::vector<std::vector<int>> nested_ints;
    RapidJsonDeserializer<decltype(nested_ints)> handler(nested_ints);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(2, static_cast<int>(nested_ints.size()));

    for(int i = 0; i < 2; ++i) {
        CHECK_EQUAL(3, static_cast<int>(nested_ints[i].size()));

        for(int j = 0; j < 3; ++j) {
            CHECK_EQUAL(i * 3 + j + 1, nested_ints[i][j]);
        }
    }
} ENDCASE

STARTCASE(DeserializeKeyValueMap) {
    std::string json = R"({"key1":"value1", "key2":"value2"})";

    std::map<string, string> keyvalue;
    RapidJsonDeserializer<decltype(keyvalue)> handler(keyvalue);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(keyvalue["key1"], "value1");
    CHECK_EQUAL(keyvalue["key2"], "value2");

} ENDCASE

STARTCASE(DeserializeKeyValueMapWithObject) {
    string json = R"({"dog1":{ "id" : 1, "name" : "Ares", "balance" : 123.45}, "dog2":{ "id" : 2, "name" : "Nusse", "balance" : 234.56}})";

    map<string, Person> keyvalue;
    RapidJsonDeserializer<decltype(keyvalue)> handler(keyvalue);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(keyvalue["dog1"].name, "Ares"s);
    CHECK_EQUAL(keyvalue["dog2"].name, "Nusse"s);

} ENDCASE

STARTCASE(DeserializeMemoryLimit)
{

    Quotes q;
    q.origin = "HGG";
    q.quote = "For instance, on the planet Earth, man had always assumed that he was" "more intelligent than dolphins because he had achieved so much—the wheel, New " "York, wars and so on—whilst all the dolphins had ever done was muck about in the " "water having a good time. But conversely, the dolphins had always believed that " "they were far more intelligent than man—for precisely the same reasons.";

    std::list<Quotes> quotes;
    for(int i = 0; i < 100; ++i) {
        q.id = i;
        quotes.push_back(q);
    }

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(quotes), decltype(writer)> serializer(quotes, writer);
    serializer.Serialize();

    std::string json = s.GetString();

    quotes.clear();

    // Add a limit of approx 4000 bytes to the handler
    serialize_properties_t properties;
    properties.max_memory_consumption = 4000;
    RapidJsonDeserializer<decltype(quotes)> handler(quotes, properties);
    Reader reader;
    StringStream ss(json.c_str());

    EXPECT_THROWS_AS(reader.Parse(ss, handler), ConstraintException);
} ENDCASE

STARTCASE(MissingObjectName) {
    Person person;
    RapidJsonDeserializer<Person> handler(person);
    Reader reader;

    {
        StringStream ss( R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":{} })");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":{"first":false, "second":12345}})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":{"sub":{"sub2":{}, "teste":"yes"}}})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({"foofoo":{"sub":{"sub2":{}, "teste":"yes"}}})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({"foofoo":{"sub":{"sub2":{}, "teste":"yes"}}, "meemee":"hi"})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({"foofoo":{"sub":{"sub2":{}, "teste":"yes", "data":["teste":"gaa"]}}, "meemee":"hi"})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({"foofoo":{}, "offoff":{"data":[]}})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({"f":{})");
        reader.Parse(ss, handler);
    }

    {
        StringStream ss( R"({"a":{"b":{"c":{"d":}}}})");
        reader.Parse(ss, handler);
    }

} ENDCASE

STARTCASE(MissingPropertyName) {
    Person person;
    std::string json = R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":"foo", "oofoof":"oof" })";

    RapidJsonDeserializer<Person> handler(person);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);
} ENDCASE

STARTCASE(SkipMissingObjectNameNotAllowed) {
    Person person;
    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    RapidJsonDeserializer<Person> handler(person, sprop);
    Reader reader;

    {
        StringStream ss( R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":{} })");
        EXPECT_THROWS_AS(reader.Parse(ss, handler), UnknownPropertyException);
    }

    {
        StringStream ss( R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":{"first":false, "second":12345}})");
        EXPECT_THROWS_AS(reader.Parse(ss, handler), UnknownPropertyException);
    }

    {
        StringStream ss( R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":{"sub":{"sub2":{}, "teste":"yes"}}})");
        EXPECT_THROWS_AS(reader.Parse(ss, handler), UnknownPropertyException);
    }
} ENDCASE

STARTCASE(MissingPropertyNameNotAllowed) {
    Person person;
    std::string json = R"({ "id" : 100, "name" : "John Longdue Doe", "balance" : 123.45, "foofoo":"foo", "oofoof":"oof" })";

    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    RapidJsonDeserializer<Person> handler(person, sprop);
    Reader reader;
    StringStream ss(json.c_str());
    EXPECT_THROWS_AS(reader.Parse(ss, handler), UnknownPropertyException);
} ENDCASE

#if (__cplusplus >= 201703L)
STARTCASE(DesearializeOptionalBoolEmpty) {
    House house;
    std::string json = R"({ "is_enabled": null })"; // No value

    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    RapidJsonDeserializer<House> handler(house, sprop);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);
    CHECK_EQUAL(house.is_enabled.has_value(), false);
} ENDCASE

STARTCASE(DesearializeOptionalBoolTrue) {
    House house;
    std::string json = R"({ "is_enabled": true })"; // No value

    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    RapidJsonDeserializer<House> handler(house, sprop);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);
    CHECK_EQUAL(house.is_enabled.has_value(), true);
    CHECK_EQUAL(house.is_enabled.value(), true);
} ENDCASE

STARTCASE(DesearializeOptionalBoolFalse) {
    House house;
    std::string json = R"({ "is_enabled": false })"; // No value

    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    RapidJsonDeserializer<House> handler(house, sprop);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);
    CHECK_EQUAL(house.is_enabled.has_value(), true);
    CHECK_EQUAL(house.is_enabled.value(), false);
} ENDCASE

STARTCASE(DesearializeOptionalObjctEmpty) {
    House house;
    house.person = Person{1, "foo", 0.0};
    std::string json = R"({ "person": null })"; // No value

    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    RapidJsonDeserializer<House> handler(house, sprop);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);
    CHECK_EQUAL(house.person.has_value(), false);
} ENDCASE

STARTCASE(DesearializeOptionalObjctAssign) {
    House house;
    std::string json = R"({ "person": { "id" : 100, "name" : "John Doe", "balance" : 123.45 }})";

    serialize_properties_t sprop;
    sprop.ignore_unknown_properties = false;
    SerializeFromJson(house, json, sprop);

    CHECK_EQUAL(house.person.has_value(), true);
    CHECK_EQUAL(house.person->id, 100);
    CHECK_EQUAL(house.person->name, "John Doe");
    CHECK_EQUAL(house.person->balance, 123.45);
} ENDCASE

STARTCASE(SerializeOptionalAllEmptyShowEmpty) {
    House house;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(false);
    serializer.Serialize();

    CHECK_EQUAL(R"({"is_enabled":null,"person":null,"pet":null})"s,
                s.GetString());

} ENDCASE


STARTCASE(SerializeOptionalAllEmpty) {
    House house;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeOptionalBoolTrue) {
    House house;
    house.is_enabled = true;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"is_enabled":true})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeOptionalBoolFalse) {
    House house;
    house.is_enabled = false;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"is_enabled":false})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeOptionalObjectWithData) {
    House house;
    house.person = Person{};
    house.person->id = 123;
    house.person->name = "Donald the looser";
    house.person->balance = 123.45;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"person":{"id":123,"name":"Donald the looser","balance":123.45}})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeOptionalObjectWithRecursiveOptionalNoData) {
    House house;
    house.person = Person{};
    house.person->id = 123;
    house.person->name = "Donald the looser";
    house.person->balance = 123.45;
    house.pet = Pet{};
    house.pet->name = "Fido";
    house.pet->kind = "Dog";

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"person":{"id":123,"name":"Donald the looser","balance":123.45},"pet":{"name":"Fido","kind":"Dog"}})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeOptionalObjectWithRecursiveOptionalData) {
    House house;
    house.person = Person{};
    house.person->id = 123;
    house.person->name = "Donald the looser";
    house.person->balance = 123.45;
    house.pet = Pet{};
    house.pet->name = "Fido";
    house.pet->kind = "Dog";
    house.pet->friends = "PusPus the Cat";

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(house), decltype(writer)>
        serializer(house, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"person":{"id":123,"name":"Donald the looser","balance":123.45},"pet":{"name":"Fido","kind":"Dog","friends":"PusPus the Cat"}})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeIgnoreEmptyString) {
    Pet pet;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(pet), decltype(writer)>
        serializer(pet, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({})"s,
                s.GetString());

} ENDCASE

STARTCASE(SerializeEmptyOptionalWithZeroValue) {

    Number data;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(data), decltype(writer)>
        serializer(data, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({})"s, s.GetString());

} ENDCASE


STARTCASE(SerializeOptionalWithZeroValue) {

    Number data;
    data.value = 0;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(data), decltype(writer)>
        serializer(data, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"value":0})"s, s.GetString());

} ENDCASE

STARTCASE(SerializeOptionalWithEmptyStringValue) {

    String data;
    data.value = "";

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(data), decltype(writer)>
        serializer(data, writer);

    serializer.IgnoreEmptyMembers(true);
    serializer.Serialize();

    CHECK_EQUAL(R"({"value":""})"s, s.GetString());

} ENDCASE

STARTCASE(DeserializeBoolFromStringTrue) {
    Bool bval;
    std::string json = R"({ "value" : "true" })";

    RapidJsonDeserializer<Bool> handler(bval);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(bval.value, true);
} ENDCASE

STARTCASE(DeserializeBoolFromStringFalse) {
    Bool bval{true};
    std::string json = R"({ "value" : "false" })";

    RapidJsonDeserializer<Bool> handler(bval);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(bval.value, false);
} ENDCASE


STARTCASE(DeserializeBoolFromIntTrue) {
    Bool bval;
    std::string json = R"({ "value" : 10 })";

    RapidJsonDeserializer<Bool> handler(bval);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(bval.value, true);
} ENDCASE

STARTCASE(DeserializeBoolFromIntFalse) {
    Bool bval{true};
    std::string json = R"({ "value" : 0 })";

    RapidJsonDeserializer<Bool> handler(bval);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(bval.value, false);
} ENDCASE

STARTCASE(DeserializeIntFromString1) {
    Int ival;
    std::string json = R"({ "value" : "1" })";

    RapidJsonDeserializer<Int> handler(ival);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(ival.value, 1);
} ENDCASE

STARTCASE(DeserializeNumbersFromStrings) {
    Numbers numbers;
    std::string json = R"({ "intval" : "-123", "sizetval": "55", "uint32": "123456789", "int64val": "-9876543212345", "uint64val": "123451234512345" })";

    RapidJsonDeserializer<Numbers> handler(numbers);
    Reader reader;
    StringStream ss(json.c_str());
    reader.Parse(ss, handler);

    CHECK_EQUAL(numbers.intval, -123);
    CHECK_EQUAL(numbers.sizetval, 55);
    CHECK_EQUAL(numbers.uint32, 123456789);
    CHECK_EQUAL(numbers.int64val, -9876543212345);
    CHECK_EQUAL(numbers.uint64val, 123451234512345);
} ENDCASE

STARTCASE(DeserializeWithStdToStringSpecialization) {

    DataHolder obj;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(obj), decltype(writer)>
        serializer(obj, writer);

    serializer.IgnoreEmptyMembers(false);
    serializer.Serialize();

    CHECK_EQUAL(R"({"data":"123"})"s, s.GetString());
} ENDCASE


STARTCASE(DeserializeWithoutStdToStringSpecialization) {

    NoDataHolder obj;

    StringBuffer s;
    Writer<StringBuffer> writer(s);

    RapidJsonSerializer<decltype(obj), decltype(writer)>
        serializer(obj, writer);

    serializer.IgnoreEmptyMembers(false);

    EXPECT_THROWS_AS(serializer.Serialize(), ParseException);
} ENDCASE


#endif // C++ 17

}; // lest

int main( int argc, char * argv[] )
{
    RESTC_CPP_TEST_LOGGING_SETUP("debug");

    return lest::run( specification, argc, argv );
}


 
