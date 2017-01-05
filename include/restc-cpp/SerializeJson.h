#pragma once

#include <iostream>
#include <type_traits>
#include <assert.h>
#include <stack>
#include <set>
#include <deque>

#include <boost/iterator/function_input_iterator.hpp>

#ifdef RESTC_CPP_HAVE_BOOST_TYPEINDEX
    #include <boost/type_index.hpp>

    #define RESTC_CPP_TYPENAME(type) \
        boost::typeindex::type_id<type>().pretty_name()
#else
    #define RESTC_CPP_TYPENAME(type) \
        typeid(type).name()
#endif


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
#include "restc-cpp/logging.h"
#include "restc-cpp/RapidJsonReader.h"
#include "restc-cpp/internals/for_each_member.hpp"
#include "restc-cpp/error.h"

namespace restc_cpp {

struct json_field_mapping {
    struct entry {
        std::string native_name;
        std::string json_name;
    };
    std::vector<entry> entries;

    const std::string&
    to_json_name(const std::string& name) const noexcept {
        for(const auto& entry : entries) {
            if (name.compare(entry.native_name) == 0) {
                return entry.json_name;
            }
        }
        return name;
    }

    const std::string&
    to_native_name(const std::string& name) const noexcept {
        for(const auto& entry : entries) {
            if (name.compare(entry.json_name) == 0) {
                return entry.native_name;
            }
        }
        return name;
    }
};

struct serialize_properties {
    bool ignore_empty_fileds = false;
    const std::set<std::string> *excluded_names = nullptr;
    const json_field_mapping *name_mapping = nullptr;

    bool is_excluded(const std::string& name) const noexcept {
        return excluded_names
            && excluded_names->find(name) != excluded_names->end();
    }

    const std::string& map_name_to_json(const std::string& name) const noexcept {
        if (name_mapping == nullptr)
            return name;
        return name_mapping->to_json_name(name);
    }
};

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

template <typename T>
struct type_conv
{
	using const_field_type_t = const T;
	using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
	using noconst_ref_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

	/*static noconst_ref_type_t& to_nc_ref(const T& v) {
		return  std::const_cast<noconst_ref_type_t>(v);
	}*/
};

template <typename T, typename fnT>
struct on_name_and_value
{
    on_name_and_value(fnT fn)
    : fn_{fn}
    {
    }

    template <typename valT>
    void operator () (const char* name, const valT& val) const {
        fn_(name, val);
    }

    void for_each_member(const T& instance) {
        restc_cpp::for_each_member(instance, *this);
    }

private:
    fnT fn_;
};

#ifndef _MSC_VER // g++, clang

template <typename varT, typename valT,
    typename std::enable_if<
        ((std::is_integral<varT>::value && std::is_integral<valT>::value)
            || (std::is_floating_point<varT>::value && std::is_floating_point<valT>::value))
        && !std::is_assignable<varT&, const valT&>::value
        >::type* = nullptr>
void assign_value(varT& var, const valT& val) {
	static_assert(!std::is_assignable<varT&, valT>::value, "assignable");
    var = static_cast<varT>(val);
}

template <typename varT, typename valT,
    typename std::enable_if<
        !((std::is_integral<varT>::value && std::is_integral<valT>::value)
            || (std::is_floating_point<varT>::value && std::is_floating_point<valT>::value))
        && std::is_assignable<varT&, const valT&>::value
        >::type* = nullptr>
void assign_value(varT& var, const valT& val) {

    var = val;
}

template <typename varT, typename valT,
    typename std::enable_if<
        !std::is_assignable<varT&, valT>::value
        && !((std::is_integral<varT>::value && std::is_integral<valT>::value)
            || (std::is_floating_point<varT>::value && std::is_floating_point<valT>::value))
        >::type* = nullptr>
void assign_value(varT& var, const valT& val) {
    assert(false);
    throw ParseException("assign_value: Invalid data conversion");
}


#else

// The C++ compiler from hell cannot select the correct template from the generics above.

template <typename varT, typename valT>
void assign_value(varT var, valT val) {
    assert(false);
    throw ParseException("assign_value: Invalid data conversion");
}

template <>
void assign_value(int& var, const int& val) {
	var = val;
}

template <>
void assign_value(unsigned int& var, const int& val) {
	var = static_cast<unsigned int>(val);
}

template <>
void assign_value(unsigned int& var, const unsigned int& val) {
	var = val;
}

template <>
void assign_value(int& var, const unsigned int& val) {
	var = static_cast<int>(val);
}

template <>
void assign_value(int& var, const bool& val) {
	var = static_cast<int>(val);
}

template <>
void assign_value(unsigned int& var, const bool& val) {
	var = static_cast<int>(val);
}

template <>
void assign_value(std::int64_t& var, const int& val) {
	var = val;
}

template <>
void assign_value(std::int64_t& var, const unsigned int& val) {
	var = static_cast<std::int64_t>(val);
}

template <>
void assign_value(std::int64_t& var, const std::int64_t& val) {
	var = val;
}

template <>
void assign_value(std::int64_t& var, const std::uint64_t& val) {
	var = static_cast<std::int64_t>(val);
}

template <>
void assign_value(std::uint64_t& var, const int& val) {
	var = static_cast<std::uint64_t>(val);
}

template <>
void assign_value(std::uint64_t& var, const unsigned int& val) {
	var = val;
}

template <>
void assign_value(std::uint64_t& var, const std::int64_t& val) {
	var = static_cast<std::uint64_t>(val);
}

template <>
void assign_value(std::uint64_t& var, const std::uint64_t& val) {
	var = val;
}

template <>
void assign_value(bool& var, const int& val) {
	var = val ? true : false;
}

template <>
void assign_value(bool& var, const unsigned int& val) {
	var = val ? true : false;
}

template <>
void assign_value(double& var, const double& val) {
	var = val;
}

template <>
void assign_value(float& var, const double& val) {
	var = static_cast<float>(val);
}

template <>
void assign_value(short& var, const int& val) {
	var = static_cast<short>(val);
}

template <>
void assign_value(short& var, const unsigned int& val) {
	var = static_cast<short>(val);
}

template <>
void assign_value(unsigned short& var, const int& val) {
	var = static_cast<unsigned short>(val);
}

template <>
void assign_value(unsigned short& var, const unsigned int& val) {
	var = static_cast<unsigned short>(val);
}

template <>
void assign_value(char& var, const int& val) {
	var = static_cast<char>(val);
}

template <>
void assign_value(char& var, const unsigned int& val) {
	var = static_cast<char>(val);
}

template <>
void assign_value(unsigned char& var, const int& val) {
	var = static_cast<unsigned char>(val);
}

template <>
void assign_value(unsigned char& var, const unsigned int& val) {
	var = static_cast<unsigned char>(val);
}

template <>
void assign_value(std::string& var, const std::string& val) {
	var = val;
}
#endif // Compiler from hell

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

template <typename T,typename Alloc>
struct is_container<std::deque<T,Alloc> > {
    constexpr static const bool value = true;
    using data_t = T;
};

} // anonymous namespace

template <typename T>
class RapidJsonDeserializer : public RapidJsonDeserializerBase {
public:
    using data_t = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    enum class State { INIT, IN_OBJECT, IN_ARRAY, RECURSED, DONE };


    RapidJsonDeserializer(data_t& object,
                          RapidJsonDeserializerBase *parent = nullptr,
                          const json_field_mapping *nameMapping = nullptr)
    : RapidJsonDeserializerBase(parent)
    , object_{object}
    , name_mapping_{nameMapping}
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
		return true;
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

        using const_field_type_t = decltype(item);
        using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
        using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

        auto& value = const_cast<field_type_t&>(item);

        recursed_to_ = std::make_unique<RapidJsonDeserializer<field_type_t>>(
            value, this, name_mapping_);
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

        auto fn = [&](const char *name, auto& val) {
            if (found) {
                return;
            }

            if (strcmp(name, current_name_.c_str()) == 0) {
                using const_field_type_t = decltype(val);
                using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;

                /* Very obscure. g++ 5.3 is unable to resolve the symbol below
                 * without "this". I don't know if this is according to the
                 * standard or a bug. It works without in clang 3.6.
                 */
                this->DoRecurseToMember<native_field_type_t>(val);
                found = true;
            }

        };

        on_name_and_value<dataT, decltype(fn)> handler(fn);
        handler.for_each_member(object_);

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
    bool SetValueOnMember(const argT& new_value,
        typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0) {
        assert(!current_name_.empty());

        bool found = false;

        auto fn = [&](const char *name, auto& val) {
            if (found) {
                return;
            }

            if (strcmp(name, current_name_.c_str()) == 0) {

                const auto& new_value_ = new_value;

                using const_field_type_t = decltype(val);
                using native_field_type_t = typename std::remove_const<
                    typename std::remove_reference<const_field_type_t>::type>::type;
                using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

                auto& value = const_cast<field_type_t&>(val);

                assign_value<decltype(value), decltype(new_value_)>(value, new_value_);
                found = true;
            }
        };

        on_name_and_value<dataT, decltype(fn)> handler(fn);
        handler.for_each_member(object_);

        current_name_.clear();
        return true;
    }

    template<typename dataT, typename argT>
    bool SetValueOnMember(argT val,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0) {

        RESTC_CPP_LOG_ERROR << RESTC_CPP_TYPENAME(dataT)
            << " BAD SetValueOnMember: ";

        assert(false);
        return true;
    }

    template<typename dataT, typename argT>
    void SetValueInArray(const argT& val,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<typename dataT::value_type>::value
            && is_container<dataT>::value
            >::type* = 0) {

		object_.push_back({});
        assign_value<decltype(object_.back()), decltype(val)>(object_.back(), val);
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

        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " SetValue: " << current_name_;

        if (state_ == State::IN_OBJECT) {
            return SetValueOnMember<data_t>(val);
        } else if (state_ == State::IN_ARRAY) {
            SetValueInArray<data_t>(val);
            return true;
        }
        assert(false && "Invalid state for setting a value");
		return true;
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
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoStartObject: " << current_name_;

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

        if (name_mapping_ == nullptr) {
            current_name_.assign(str, length);
        } else {
            std::string name{str, length};
            current_name_ = name_mapping_->to_native_name(name);
        }

        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoKey: " << current_name_;
        return true;
    }

    bool DoEndObject(std::size_t memberCount) {

        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoEndObject: " << current_name_;

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

        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoStartArray: " << current_name_;

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
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoEndArray: " << current_name_;

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
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << "OnChildIsDone";

        assert(state_ == State::RECURSED);
        assert(!saved_state_.empty());

        state_ = saved_state_.top();
        saved_state_.pop();
        recursed_to_.reset();
    }

private:
    data_t& object_;
    const json_field_mapping *name_mapping_ = nullptr;
    std::string current_name_;
    //decltype(with_names(object_)) struct_members_;
    State state_ = State::INIT;
    std::stack<State> saved_state_;
    std::unique_ptr<RapidJsonDeserializerBase> recursed_to_;
};


namespace {
    template <typename T>
    constexpr bool is_empty_field_(const T& value,
        typename std::enable_if<
            !std::is_integral<T>::value
            && !std::is_floating_point<T>::value
            && !std::is_same<T, std::string>::value
            && !is_container<T>::value
            >::type* = 0) {

        return false;
    }

    template <typename T>
    constexpr bool is_empty_field_(const T& value,
        typename std::enable_if<
            std::is_integral<T>::value
            || std::is_floating_point<T>::value
            >::type* = 0) {
        return value == T{};
    }

    template <typename T>
    constexpr bool is_empty_field_(const T& value,
        typename std::enable_if<
            std::is_same<T, std::string>::value || is_container<T>::value
            >::type* = 0) {
        return value.empty();
    }

    template <typename T>
    constexpr bool is_empty_field(T&& value) {
        using data_type = typename std::remove_const<typename std::remove_reference<T>::type>::type;
        return is_empty_field_<data_type>(value);
    }

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
                      const serialize_properties& properties,
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
                      const serialize_properties& properties,
        typename std::enable_if<
            std::is_integral<dataT>::value
            || std::is_floating_point<dataT>::value
            >::type* = 0) {

        do_serialize_integral(object, serializer);
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
                      const serialize_properties& properties,
        typename std::enable_if<
            std::is_same<dataT, std::string>::value
            >::type* = 0) {

        serializer.String(object.c_str(), object.size(), true);
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
                      const serialize_properties& properties,
         typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type* = 0);

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
                      const serialize_properties& properties,
        typename std::enable_if<
            is_container<dataT>::value
            >::type* = 0) {
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
            << " StartArray: ";

        serializer.StartArray();

        for(const auto& v: object) {

            using native_field_type_t = typename std::remove_const<
                typename std::remove_reference<decltype(v)>::type>::type;

            do_serialize<native_field_type_t>(v, serializer, properties);
        }
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
            << " EndArray: ";
        serializer.EndArray();
    };

    template <typename dataT, typename serializerT>
    void do_serialize(const dataT& object, serializerT& serializer,
                      const serialize_properties& properties,
         typename std::enable_if<
            boost::fusion::traits::is_sequence<dataT>::value
            >::type*) {

        serializer.StartObject();
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
            << " StartObject: ";
           auto fn = [&](const char *name, auto& val) {
            RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
                << " Key: " << name;
            if (properties.ignore_empty_fileds) {
                if (is_empty_field(val)) {
            RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
                << " ignoring empty field.";
                    return;
                }
            }

            if (properties.excluded_names
                && properties.is_excluded(name)) {
                RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
                    << " ignoring excluded field.";
                return;
            }

            serializer.Key(properties.map_name_to_json(name).c_str());

            using const_field_type_t = decltype(val);
            using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
            using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

            auto& const_value = val;
            auto& value = const_cast<field_type_t&>(const_value);

            do_serialize<native_field_type_t>(value, serializer, properties);

        };

        on_name_and_value<dataT, decltype(fn)> handler(fn);
        handler.for_each_member(object);

        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
            << " EndObject: ";
        serializer.EndObject();
    };
}

template <typename objectT, typename serializerT>
class RapidJsonSerializer
{
public:
    using data_t = typename std::remove_const<typename std::remove_reference<objectT>::type>::type;

    RapidJsonSerializer(const data_t& object, serializerT& serializer)
    : object_{object}, serializer_{serializer}
    {
    }

    /*! Recursively serialize the C++ object to the json serializer
     *
     * See https://github.com/miloyip/rapidjson/blob/master/doc/sax.md#writer-writer
     */
    void Serialize() {
        do_serialize<data_t>(object_, serializer_, properties_);
    }

    void IgnoreEmptyMembers(bool ignore = true) {
        properties_.ignore_empty_fileds = ignore;
    }

    // Set to nullptr to disable lookup
    void ExcludeNames(const std::set<std::string> *names) {
        properties_.excluded_names = names;
    }

    void SetNameMapping(const json_field_mapping *mapping) {
        properties_.name_mapping = mapping;
    }

private:

    const data_t& object_;
    serializerT& serializer_;
    serialize_properties properties_;
};

/*! Serialize as a container
 *
 * Serialize only one list-object at a time. Follow
 * the InputIterator contract.
 * ref: http://en.cppreference.com/w/cpp/concept/InputIterator
 */
template <typename objectT>
class IteratorFromJsonSerializer
{
public:
    using data_t = typename std::remove_const<typename std::remove_reference<objectT>::type>::type;

    class Iterator : public std::iterator<
        std::input_iterator_tag,
        data_t,
        std::ptrdiff_t,
        const data_t *,
        data_t&> {

    public:
        Iterator() {}

        /*! Copy constructor
         * \note This constructor mailnly is supplied to make it++ work.
         *      It is not optimized for performance.
         *      As always, prefer ++it.
         */
        Iterator(const Iterator& it)
        : owner_{it.owner_}
        {
            if (it.data_) {
                data_ = std::make_unique<objectT>(*it.data_);
            }
        }

        Iterator(Iterator&& it)
        : owner_{it.owner_}, data_{move(it.data_)} {}

        Iterator(IteratorFromJsonSerializer *owner)
        : owner_{owner} {}

        Iterator& operator++() {
            prepare();
            fetch();
            return *this;
        }

        Iterator operator++(int) {
            prepare();
            Iterator retval = *this;
            fetch();
            return retval;
        }

        Iterator& operator = (const Iterator& it) {
            owner_ = it.owner_;
            if (it.data_) {
                data_ = std::make_unique<objectT>(*it.data_);
            }
            return *this;
        }

        Iterator& operator = (Iterator&& it) {
            owner_ = it.owner_;
            it.data_ = move(it.data_);
        }

        bool operator == (const Iterator& other) const {
            prepare();
            return data_.get() == other.data_.get();
        }

        bool operator != (const Iterator& other) const {
            return ! operator == (other);
        }

        data_t& operator*() const {
            prepare();
            return get();
        }

        data_t *operator -> () const {
            prepare();
            return data_.get();
        }

    private:
        void prepare() const {
            if (virgin_) {
                virgin_ = false;

                const_cast<Iterator *>(this)->fetch();
            }
        }

        void fetch() {
            if (owner_) {
                data_ = owner_->fetch();
            } else {
                throw CannotIncrementEndException(
                    "this is an end() iterator.");
            }
        }

        data_t& get() const {
            if (!data_) {
                throw NoDataException("data_ is empty");
            }
            return *data_;
        }

        IteratorFromJsonSerializer *owner_ = nullptr;
        std::unique_ptr<objectT> data_;
        mutable bool virgin_ = true;
    };


    using iterator_t = Iterator;

    IteratorFromJsonSerializer(
        Reply& reply,
        const json_field_mapping *nameMapper = nullptr)
    : reply_stream_{reply}, name_mapper_{nameMapper} {}

    iterator_t begin() {
        return Iterator{this};
    }

    iterator_t end() {
        return Iterator{};
    }

private:
    std::unique_ptr<objectT> fetch() {

        if(state_ == State::PRE) {
            const auto ch = reply_stream_.Take();
            if (ch != '[') {
                throw ParseException("Expected leading json '{'");
            }
            state_ = State::ITERATING;
        }

        if (state_ == State::ITERATING) {
            while(true) {
                const auto ch = reply_stream_.Peek();
                if ((ch == ' ') || (ch == '\t') || (ch == ',') || (ch == '\n') || (ch == '\r')) {
                    reply_stream_.Take();
                    continue;
                } else if (ch == '{') {
                    auto data = std::make_unique<objectT>();
                    RapidJsonDeserializer<objectT> handler(
                        *data, nullptr, name_mapper_);
                    json_reader_.Parse(reply_stream_, handler);
                    return move(data);
                } else if (ch == ']') {
                    reply_stream_.Take();
                    state_ = State::DONE;
                    break;
                } else {
                    throw ParseException("Unexpected character in input stream");
                }
            }
        }

        assert(state_ == State::DONE);
        return {};
    }

    enum class State { PRE, ITERATING, DONE };
    State state_ = State::PRE;
    RapidJsonReader reply_stream_;
    rapidjson::Reader json_reader_;
    const json_field_mapping *name_mapper_;
};

template <typename dataT>
void SerializeFromJson(dataT& rootData,
                       Reply& reply,
                       const json_field_mapping *nameMapper = nullptr) {
    RapidJsonDeserializer<dataT> handler(rootData, nullptr, nameMapper);
    RapidJsonReader reply_stream(reply);
    rapidjson::Reader json_reader;
    json_reader.Parse(reply_stream, handler);
}

template <typename dataT>
void SerializeFromJson(dataT& rootData,
                       std::unique_ptr<Reply>&& reply,
                       const json_field_mapping *nameMapper = nullptr) {
   SerializeFromJson(rootData, *reply, nameMapper);
}

} // namespace

