#pragma once

#ifndef RESTC_CPP_SERIALIZE_H_
#define RESTC_CPP_SERIALIZE_H_

#include <assert.h>
#include <type_traits>
#include <list>
#include <stack>

#include <boost/lexical_cast.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RapidJsonReader.h"


namespace restc_cpp {

template <typename T>
struct Serialize
{
    using set_fun_t = std::function<void (T&, std::string&&)>;
    using get_fun_t = std::function<std::string(const T&)>;

    struct Field {
        set_fun_t set;
        get_fun_t get;
    };

    using obj_t = T;
    using fields_map_t = std::map<std::string, Field>;
    using fields_t = typename fields_map_t::value_type;
    using init_t = std::initializer_list<fields_t>;


    void SetValue(const std::string& name, T& obj, std::string&& value) {
        auto field_it = fields_.find(name);
        if (field_it == fields_.end()) {
            throw std::invalid_argument(name);
        }
        auto& field = field_it->second;
        if (field.set) {
            field.set(obj, std::move(value));
        }
    }

    Serialize(init_t init_fields)
    : fields_(init_fields) {}

private:

    fields_map_t fields_;
};

/*! Base class that satisfies the requirements from rapidjson */
class RapidJsonHandler {
public:
    RapidJsonHandler(RapidJsonHandler *root)
    : root_{root}
    {
    }

    virtual ~RapidJsonHandler()
    {
    }

    virtual void Push(const std::shared_ptr<RapidJsonHandler>& handler) {
        root_->Push(handler);
    }

    virtual void Pop() {
        root_->Pop();
    }

    virtual bool Null() = 0;

    virtual bool Bool(bool b) = 0;

    virtual bool Int(int i) = 0;

    virtual bool Uint(unsigned u) = 0;

    virtual bool Int64(int64_t i) = 0;

    virtual bool Uint64(uint64_t u) = 0;

    virtual bool Double(double d) = 0;

    virtual bool String(const char* str, std::size_t Tlength, bool copy) = 0;

    virtual bool RawNumber(const char* str, std::size_t length, bool copy) = 0;

    virtual bool StartObject() = 0;

    virtual bool Key(const char* str, std::size_t length, bool copy) = 0;

    virtual bool EndObject(std::size_t memberCount) = 0;

    virtual bool StartArray() = 0;

    virtual bool EndArray(std::size_t elementCount) = 0;

private:
    RapidJsonHandler *root_;
};

class Reply;

/*! Proxy handler to allow nested objects */
class RootRapidJsonHandler : public RapidJsonHandler
{

    RapidJsonHandler& Current() {
        assert(!handlers_.empty());
        return *handlers_.top();
    }
public:
    RootRapidJsonHandler()
    : RapidJsonHandler(nullptr)
    {
    }

    ~RootRapidJsonHandler() = default;

    virtual void Push(const std::shared_ptr<RapidJsonHandler>& handler) override {
        handlers_.push(handler);
    }

    virtual void Pop() override {
        handlers_.pop();
    }

    bool Null() override {
        return Current().Null();
    }

    bool Bool(bool b) override {
        return Current().Bool(b);
    }

    bool Int(int i) override {
        return Current().Int(i);
    }

    bool Uint(unsigned u) override {
        return Current().Uint(u);
    }

    bool Int64(int64_t i) override {
        return Current().Int64(i);
    }

    bool Uint64(uint64_t u) override {
        return Current().Uint64(u);
    }

    bool Double(double d) override {
        return Current().Double(d);
    }

    bool String(const char* str, std::size_t length, bool copy) override {
        return Current().String(str, length, copy);
    }

    bool RawNumber(const char* str, std::size_t length, bool copy) override {
        return Current().RawNumber(str, length, copy);
    }

    bool StartObject() override {
        return Current().StartObject();
    }

    bool Key(const char* str, std::size_t length, bool copy) override {
        return Current().Key(str, length, copy);
    }

    bool EndObject(std::size_t memberCount) override {
        return Current().EndObject(memberCount);
    }

    bool StartArray() override {
        return Current().StartArray();
    }

    bool EndArray(std::size_t elementCount) override {
        return Current().EndArray(elementCount);
    }

    std::stack<std::shared_ptr<RapidJsonHandler>> handlers_;

    void FetchAll(std::unique_ptr<Reply>&& reply) {
        FetchAll(*reply);
    }

    void FetchAll(Reply& reply) {
        rapidjson::Reader json_reader;
        RapidJsonReader reply_stream(reply);
        json_reader.Parse(reply_stream, *this);
    }

private:
    std::unique_ptr<RapidJsonReader> reader_;
};

/*! Handler for cases where the json is an array of uniform objects */
template <typename T,
    typename SerializeT = Serialize<T>,
    typename ListT = std::list<T>>
class RapidJsonHandlerObjectArray : public RapidJsonHandler {
public:
    using list_t = ListT;

    RapidJsonHandlerObjectArray(RootRapidJsonHandler *root,
                                list_t& objects,
                                SerializeT& serializer)
    : RapidJsonHandler(root), objects_{objects}, serializer_{serializer}
    {
    }

    template<typename argT>
    bool SetValue(argT val) {
        assert(!current_name_.empty());
        assert(is_in_object_);
        assert(!objects_.empty());


        serializer_.SetValue(current_name_, objects_.back(),
                             boost::lexical_cast<std::string>(val));

        current_name_.clear();

        return true;
    }

    bool Null() override {
        const static std::string empty;
        return SetValue(empty);
    }

    bool Bool(bool b) override {
        return SetValue(b);
    }

    bool Int(int i) override {
        return SetValue(i);
    }

    bool Uint(unsigned u) override {
        return SetValue(u);
    }

    bool Int64(int64_t i) override {
        return SetValue(i);
    }

    bool Uint64(uint64_t u) override {
        return SetValue(u);
    }

    bool Double(double d) override {
        return SetValue(d);
    }

    bool String(const char* str, std::size_t length, bool copy) override {
        return SetValue(std::string(str, length));
    }

    bool RawNumber(const char* str, std::size_t length, bool copy) override {
        assert(false);
    }

    bool StartObject() override {
        assert(is_in_object_ == false);
        is_in_object_ = true;
        objects_.push_back({});
        return true;
    }

    bool Key(const char* str, std::size_t length, bool copy) override {
        assert(current_name_.empty());
        current_name_.assign(str, length);
        return true;
    }

    bool EndObject(std::size_t memberCount) override {
        assert(is_in_object_ == true);
        is_in_object_ = false;
        current_name_.clear();
        return true;
    }

    bool StartArray() override {
        return true;
    }

    bool EndArray(std::size_t elementCount) override {
        return true;
    }

private:
    list_t& objects_;
    std::string current_name_;
    bool is_in_object_ = false;
    SerializeT& serializer_;
    std::unique_ptr<RootRapidJsonHandler> root_handler_instance_;
};

/*! Handler for cases where the json is an object */
template <typename T, typename SerializeT = Serialize<T>>
class RapidJsonHandlerObject : public RapidJsonHandler {
public:
    using data_t = T;

    RapidJsonHandlerObject(data_t& object, SerializeT& serializer)
    : object_{object}, serializer_{serializer}
    {
    }

    template<typename argT>
    bool SetValue(argT val) {
        assert(!current_name_.empty());
        assert(is_in_object_);
        serializer_.SetValue(current_name_, object_,
                             boost::lexical_cast<std::string>(val));
        current_name_.clear();
        return true;
    }

    bool Null() override {
        const static std::string empty;
        return SetValue(empty);
    }

    bool Bool(bool b) override {
        return SetValue(b);
    }

    bool Int(int i) override {
        return SetValue(i);
    }

    bool Uint(unsigned u) override {
        return SetValue(u);
    }

    bool Int64(int64_t i) override {
        return SetValue(i);
    }

    bool Uint64(uint64_t u) override {
        return SetValue(u);
    }

    bool Double(double d) override {
        return SetValue(d);
    }

    bool String(const char* str, std::size_t length, bool copy) override {
        return SetValue(std::string(str, length));
    }

    bool RawNumber(const char* str, std::size_t length, bool copy) override {
        assert(false);
    }

    bool StartObject() override {
        assert(is_in_object_ == false);
        is_in_object_ = true;
        // TODO: Construct the object if nested
        return true;
    }

    bool Key(const char* str, std::size_t length, bool copy) override {
        assert(current_name_.empty());
        current_name_.assign(str, length);
        return true;
    }

    bool EndObject(std::size_t memberCount) override {
        assert(is_in_object_ == true);
        is_in_object_ = false;
        current_name_.clear();
        return true;
    }

    bool StartArray() override {
        assert(false);
        return true;
    }

    bool EndArray(std::size_t elementCount) override {
        assert(false);
        return true;
    }

private:
    data_t& object_;
    std::string current_name_;
    bool is_in_object_ = false;
    SerializeT& serializer_;
};


template <typename HandlerT, typename dataT, typename serializerT>
std::shared_ptr<RapidJsonHandler> CreateDataRapidJsonHandler(
    std::shared_ptr<RootRapidJsonHandler>& root,
    dataT& instance, serializerT& serializer) {

    return std::make_shared<HandlerT>(root.get(), instance, serializer);
}

template <typename handlerT, typename dataT, typename serializerT>
std::shared_ptr<RootRapidJsonHandler> CreateRootRapidJsonHandler(
    dataT& instance, serializerT& serializer) {
    auto root = std::make_shared<RootRapidJsonHandler>();
    root->Push(CreateDataRapidJsonHandler<handlerT>(root, instance, serializer));
    return root;
}


} // restc_cpp

// Json nam e is different fromthe C++ member name
#define DECL_FIELD_JN(class, type, jsonname, fieldname) \
{ #jsonname, {\
    [](class& o, std::string&& v) {o.fieldname = boost::lexical_cast<type>(v);}, \
    [](const class& o) {return boost::lexical_cast<std::string>(o.fieldname);}}}

#define DECL_FIELD(class, type, fieldname) DECL_FIELD_JN(class, type, fieldname, fieldname)

#endif // RESTC_CPP_SERIALIZE_H_
