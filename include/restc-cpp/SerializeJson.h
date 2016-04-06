#pragma once

#include <iostream>
#include <type_traits>
#include <assert.h>
#include <stack>

#include <boost/type_index.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/segments.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/container.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RapidJsonReader.h"



#define RESTC_CPP_LOG std::clog

namespace restc_cpp {

/*! Base class that satisfies the requirements from rapidjson */
class RapidJsonDeserializerBase {
public:
    RapidJsonDeserializerBase(RapidJsonDeserializerBase *parent)
    : parent_{parent}
    {
    }

    virtual ~RapidJsonDeserializerBase()
    {
    }

    virtual void Push(const std::shared_ptr<RapidJsonDeserializerBase>& handler) {
        parent_->Push(handler);
    }

    virtual void Pop() {
        parent_->Pop();
    }

    // Outer interface
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

    virtual void OnChildIsDone() {};
    RapidJsonDeserializerBase& GetParent() {
        assert(parent_ != nullptr);
        return *parent_;
    }
    bool HaveParent() const noexcept {
        return parent_ != nullptr;
    }

private:
    RapidJsonDeserializerBase *parent_;
};


namespace {

template<class Struct>
auto with_names(const Struct& s)
{
    using range = boost::mpl::range_c<int, 0, (boost::fusion::result_of::size<Struct>())>;
    static auto names = boost::fusion::transform(range(), [](auto i) -> std::string
    {
        return boost::fusion::extension::struct_member_name<Struct, i>::call();
    });
    return boost::fusion::zip(s, names);
}


template<class T>
auto& get_value(const T& x)
{
    return boost::fusion::at_c<0>(x);
}

template<class T>
std::string get_name(const T& x)
{
    return boost::fusion::at_c<1>(x);
}

void assign_value(int& var, unsigned val) {
    var = static_cast<int>(val);
}

void assign_value(std::int64_t& var, std::uint64_t val) {
    var = static_cast<int64_t>(val);
}

// Double& = double fails std::is_assignable
void assign_value(double& var, double val) {
    var = val;
}

template <typename varT, typename valT,
    typename std::enable_if<
        std::is_assignable<varT, valT>::value
        >::type* = nullptr>
void assign_value(varT& var, valT val) {
    var = val;
}

template <typename varT, typename valT,
    typename std::enable_if<!std::is_assignable<varT, valT>::value>::type* = nullptr>
void assign_value(varT& var, valT val) {
    assert(false);
}

template <typename T>
struct is_container {
    constexpr static const bool value = false;
};

template <typename T,typename Alloc>
struct is_container<std::vector<T,Alloc> > {
    constexpr static const bool value = true;
};

template <typename T,typename Alloc>
struct is_container<std::list<T,Alloc> > {
    constexpr static const bool value = true;
    using data_t = T;
};

} // anonymous namespace

template <typename T>
class RapidJsonDeserializer : public RapidJsonDeserializerBase {
public:
    using data_t = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    enum class State { INIT, IN_OBJECT, IN_ARRAY, RECURSED, DONE };


    RapidJsonDeserializer(data_t& object, RapidJsonDeserializerBase *parent = nullptr)
    : RapidJsonDeserializerBase(parent)
    , object_{object}
    //, struct_members_{with_names(object_)}
    {

    }

    bool Null() override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Null() : DoNull();
    }

    bool Bool(bool b) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Bool(b) : DoBool(b);
    }

    bool Int(int i) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Int(i) : DoInt(i);
    }

    bool Uint(unsigned u) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Uint(u) : DoUint(u);
    }

    bool Int64(int64_t i) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Int64(i) : DoInt64(i);
    }

    bool Uint64(uint64_t u) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Uint64(u) : DoUint64(u);
    }

    bool Double(double d) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->Double(d) : DoDouble(d);
    }

    bool String(const char* str, std::size_t length, bool copy) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_
            ? recursed_to_->String(str, length, copy)
            : DoString(str, length, copy);
    }

    bool RawNumber(const char* str, std::size_t length, bool copy) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        assert(false);
    }

    bool StartObject() override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->StartObject() : DoStartObject();
    }

    bool Key(const char* str, std::size_t length, bool copy) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_
            ? recursed_to_->Key(str, length, copy)
            : DoKey(str, length, copy);
    }

    bool EndObject(std::size_t memberCount) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_
            ? recursed_to_->EndObject(memberCount)
            : DoEndObject(memberCount);
    }

    bool StartArray() override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_ ? recursed_to_->StartArray() : DoStartArray();
    }

    bool EndArray(std::size_t elementCount) override {
        assert(((state_ == State::RECURSED) && recursed_to_) || !recursed_to_);
        return recursed_to_
            ? recursed_to_->EndArray(elementCount)
            : DoEndArray(elementCount);
    }

private:


    template <typename classT, typename itemT>
    void DoRecurseToMember(itemT& item,
        typename std::enable_if<
            boost::fusion::traits::is_sequence<classT>::value
            || is_container<classT>::value
            >::type* = 0) {

        using const_field_type_t = decltype(get_value(item));
        using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
        using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

        auto& const_value = get_value(item);
        auto& value = const_cast<field_type_t&>(const_value);

        recursed_to_ = std::make_unique<RapidJsonDeserializer<field_type_t>>(value, this);
    }

    template <typename classT, typename itemT>
    void DoRecurseToMember(itemT& field,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<classT>::value
            && !is_container<classT>::value
            >::type* = 0) {
        assert(false);
    }

    // boost::fusion declared classes
    template <typename dataT>
    void RecurseToContainerValue(typename std::enable_if<
            boost::fusion::traits::is_sequence<typename dataT::value_type>::value
            && is_container<dataT>::value
            >::type* = 0) {

        object_.push_back({});

        using native_type_t = typename std::remove_const<
            typename std::remove_reference<typename dataT::value_type>::type>::type;
        recursed_to_ = std::make_unique<RapidJsonDeserializer<native_type_t>>(object_.back(), this);
        saved_state_.push(state_);
        state_ = State::RECURSED;
    }

    // Simple data types like int and string
    template <typename dataT>
    void RecurseToContainerValue(typename std::enable_if<
            !boost::fusion::traits::is_sequence<typename dataT::value_type>::value
            && is_container<dataT>::value
            >::type* = 0) {

        // Do nothing. We will push_back() the values as they arrive
    }

    template <typename dataT>
    void RecurseToContainerValue(typename std::enable_if<!is_container<dataT>::value>::type* = 0) {
        assert(false);
    }



    template <typename dataT>
    void RecurseToMember(typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0) {
        assert(!recursed_to_);
        assert(!current_name_.empty());

        bool found = false;
        boost::fusion::for_each(with_names(object_), [&](const auto item) {
            /* It's probably better to use a recursive search,
             * but this will do for now.
             */
            if (found) {
                return;
            }

            if (get_name(item).compare(current_name_) == 0) {
                using const_field_type_t = decltype(boost::fusion::at_c<0>(item));
                using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;

                /* Very obscure. g++ 5.3 is unable to resolve the symbol below
                 * without "this". I don't know if this is according to the
                 * standard or a bug. It works without in clang 3.6.
                 */
                this->DoRecurseToMember<native_field_type_t>(item);
                found = true;
            }
        });

        assert(recursed_to_);
        assert(found);
        saved_state_.push(state_);
        state_ = State::RECURSED;
        current_name_.clear();
    }

    template <typename dataT>
    void RecurseToMember( typename std::enable_if<
            !boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0) {
        assert(!recursed_to_);
    }

    template<typename dataT, typename argT>
    bool SetValueOnMember(argT val,
        typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0) {
        assert(!current_name_.empty());

        bool found = false;
        boost::fusion::for_each(with_names(object_), [&](const auto item) {
            /* It's probably better to use a recursive search,
             * but this will do for now.
             */
            if (found) {
                return;
            }

            if (get_name(item).compare(current_name_) == 0) {
                using const_field_type_t = decltype(boost::fusion::at_c<0>(item));
                using native_field_type_t = typename std::remove_const<
                    typename std::remove_reference<const_field_type_t>::type>::type;
                using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

                auto& const_value = get_value(item);
                auto& value = const_cast<field_type_t&>(const_value);

                assign_value(value, val);
                found = true;
            }
        });

        current_name_.clear();
        return true;
    }

    template<typename dataT, typename argT>
    bool SetValueOnMember(argT val,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0) {

#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<dataT>().pretty_name()
            << " BAD SetValueOnMember: " << std::endl;
#endif

        assert(false);
        return true;
    }

    template<typename dataT, typename argT>
    void SetValueInArray(argT val,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<typename dataT::value_type>::value
            && is_container<dataT>::value
            >::type* = 0) {

        object_.push_back({});
        assign_value(object_.back(), val);
    }

    template<typename dataT, typename argT>
    void SetValueInArray(argT val,
        typename std::enable_if<
            boost::fusion::traits::is_sequence<typename dataT::value_type>::value
            && is_container<dataT>::value
            >::type* = 0) {

        // We should always recurse into structs
        assert(false);
    }

    template<typename dataT, typename argT>
    void SetValueInArray(argT val,
        typename std::enable_if<
            !is_container<dataT>::value
            >::type* = 0) {

        assert(false);
    }

    template<typename argT>
    bool SetValue(argT val) {

#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << " SetValue: " << current_name_ << std::endl;
#endif

        if (state_ == State::IN_OBJECT) {
            return SetValueOnMember<data_t>(val);
        } else if (state_ == State::IN_ARRAY) {
            SetValueInArray<data_t>(val);
            return true;
        }
        assert(false && "Invalid state for setting a value");
    }

    bool DoNull() {
        // TODO: Clear value
        return true;
    }

    bool DoBool(bool b) {
        return SetValue(b);
    }

    bool DoInt(int i) {
        return SetValue(i);
    }

    bool DoUint(unsigned u) {
        return SetValue(u);
    }

    bool DoInt64(int64_t i) {
        return SetValue(i);
    }

    bool DoUint64(uint64_t u) {
        return SetValue(u);
    }

    bool DoDouble(double d) {
        return SetValue(d);
    }

    bool DoString(const char* str, std::size_t length, bool copy) {
        return SetValue(std::string(str, length));
    }

    bool DoRawNumber(const char* str, std::size_t length, bool copy) {
        assert(false);
    }

    bool DoStartObject() {
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << " DoStartObject: " << current_name_ << std::endl;
#endif

        // TODO: Recurse into nested objects
        switch (state_) {

            case State::INIT:
                state_ = State::IN_OBJECT;
                break;
            case State::IN_OBJECT:
                RecurseToMember<data_t>();
                recursed_to_->StartObject();
                break;
            case State::IN_ARRAY:
                RecurseToContainerValue<data_t>();
                recursed_to_->StartObject();
                break;
            default:
                assert(false && "Unexpected state");

        }
        return true;
    }

    bool DoKey(const char* str, std::size_t length, bool copy) {
        assert(current_name_.empty());
        current_name_.assign(str, length);
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << " DoKey: " << current_name_ << std::endl;
#endif
        return true;
    }

    bool DoEndObject(std::size_t memberCount) {
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << " DoEndObject: " << current_name_ << std::endl;
#endif

        current_name_.clear();

        switch (state_) {
            case State::IN_OBJECT:
                state_ = State::DONE;
                break;
            case State::IN_ARRAY:
                assert(false); // FIXME?
                break;
            default:
                assert(false && "Unexpected state");

        }

        if (state_ == State::DONE) {
            if (HaveParent()) {
                GetParent().OnChildIsDone();
            }
        }
        return true;
    }

    bool DoStartArray() {
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << " DoStartArray: " << current_name_ << std::endl;
#endif

        if (state_ == State::INIT) {
            state_ = State::IN_ARRAY;
        } else if (state_ == State::IN_OBJECT) {
            RecurseToMember<data_t>();
            recursed_to_->StartArray();
        } else {
            assert(false);
        }
        return true;
    }

    bool DoEndArray(std::size_t elementCount) {
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << " DoEndArray: " << current_name_ << std::endl;
#endif
        current_name_.clear();

        switch (state_) {
            case State::IN_OBJECT:
                assert(false); // FIXME?
                break;
            case State::IN_ARRAY:
                state_ = State::DONE;
                break;
            default:
                assert(false && "Unexpected state");

        }

        if (state_ == State::DONE) {
            if (HaveParent()) {
                GetParent().OnChildIsDone();
            }
        }
        return true;
    }

    void OnChildIsDone() override {
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<data_t>().pretty_name()
            << "OnChildIsDone" << std::endl;
#endif

        assert(state_ == State::RECURSED);
        assert(!saved_state_.empty());

        state_ = saved_state_.top();
        saved_state_.pop();
        recursed_to_.reset();
    }

private:
    data_t& object_;
    std::string current_name_;
    //decltype(with_names(object_)) struct_members_;
    State state_ = State::INIT;
    std::stack<State> saved_state_;
    std::unique_ptr<RapidJsonDeserializerBase> recursed_to_;
};


namespace {

    template <typename T, typename S>
    void do_serialize_integral(const T& v, S& serializer) {
        assert(false);
    }

    template <typename S>
    void do_serialize_integral(const bool& v, S& serializer) {
        serializer.Bool(v);
    }

    template <typename S>
    void do_serialize_integral(const int& v, S& serializer) {
        serializer.Int(v);
    }

    template <typename S>
    void do_serialize_integral(const unsigned int& v, S& serializer) {
        serializer.Uint(v);
    }

    template <typename S>
    void do_serialize_integral(const double& v, S& serializer) {
        serializer.Double(v);
    }

    template <typename S>
    void do_serialize_integral(const std::int64_t& v, S& serializer) {
        serializer.Int64(v);
    }

    template <typename S>
    void do_serialize_integral(const std::uint64_t& v, S& serializer) {
        serializer.Uint64(v);
    }


    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<dataT>::value
            && !std::is_integral<dataT>::value
            && !std::is_floating_point<dataT>::value
            && !std::is_same<dataT, std::string>::value
            && !is_container<dataT>::value
            >::type* = 0) {
        assert(false);
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
        typename std::enable_if<
            std::is_integral<dataT>::value
            || std::is_floating_point<dataT>::value
            >::type* = 0) {

        do_serialize_integral(object, serializer);
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
        typename std::enable_if<
            std::is_same<dataT, std::string>::value
            >::type* = 0) {

        serializer.String(object.c_str(), object.size(), true);
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
         typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0);

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
        typename std::enable_if<
            is_container<dataT>::value
            >::type* = 0) {
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<dataT>().pretty_name()
            << " StartArray: " << std::endl;
#endif
        serializer.StartArray();

        for(const auto& v: object) {

            using native_field_type_t = typename std::remove_const<
                typename std::remove_reference<decltype(v)>::type>::type;

            do_serialize<native_field_type_t>(v, serializer);
        }
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<dataT>().pretty_name()
            << " EndArray: " << std::endl;
#endif
        serializer.EndArray();
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
         typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type*) {

        serializer.StartObject();
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<dataT>().pretty_name()
            << " StartObject: " << std::endl;
#endif

        boost::fusion::for_each(with_names(object), [&](const auto item) {
            /* It's probably better to use a recursive search,
             * but this will do for now.
             */
#ifdef RESTC_CPP_LOG
            RESTC_CPP_LOG << boost::typeindex::type_id<dataT>().pretty_name()
                << " Key: " << get_name(item) << std::endl;
#endif

            serializer.Key(get_name(item).c_str());

            using const_field_type_t = decltype(get_value(item));
            using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
            using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

            auto& const_value = get_value(item);
            auto& value = const_cast<field_type_t&>(const_value);

            do_serialize<native_field_type_t>(value, serializer);

        });
#ifdef RESTC_CPP_LOG
        RESTC_CPP_LOG << boost::typeindex::type_id<dataT>().pretty_name()
            << " EndObject: " << std::endl;
#endif
        serializer.EndObject();
    };
}

template <typename objectT, typename serializerT>
class RapidJsonSerializer
{
public:
    using data_t = typename std::remove_const<typename std::remove_reference<objectT>::type>::type;

    RapidJsonSerializer(data_t& object, serializerT& serializer)
    : object_{object}, serializer_{serializer}
    {
    }

    /*! Recursively serialize the C++ object to the json serializer
     *
     * See https://github.com/miloyip/rapidjson/blob/master/doc/sax.md#writer-writer
     */
    void Serialize() {
        do_serialize<data_t>(object_, serializer_);
    }

private:

    data_t& object_;
    serializerT& serializer_;
};


template <typename dataT>
void SerializeFromJson(dataT& rootData, Reply& reply) {
    RapidJsonDeserializer<dataT> handler(rootData);
    RapidJsonReader reply_stream(reply);
    rapidjson::Reader json_reader;
    json_reader.Parse(reply_stream, handler);
}

template <typename dataT>
void SerializeFromJson(dataT& rootData, std::unique_ptr<Reply>&& reply) {
   SerializeFromJson(rootData, *reply);
}

} // namespace

