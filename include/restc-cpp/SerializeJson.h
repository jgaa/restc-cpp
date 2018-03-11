#pragma once

#include <iostream>
#include <type_traits>
#include <assert.h>
#include <stack>
#include <set>
#include <deque>
#include <map>

#include <boost/iterator/function_input_iterator.hpp>
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
#include "restc-cpp/typename.h"
#include "restc-cpp/RapidJsonWriter.h"

#include "rapidjson/writer.h"
#include "rapidjson/istreamwrapper.h"

namespace restc_cpp {

using excluded_names_t = std::set<std::string>;

/*! Mapping between C++ property names and json names.
*
* Normally we will use the same, but in some cases we
* need to appy mapping.
*/
struct JsonFieldMapping {
    struct entry {
        entry() = default;
        entry(const std::string& native, const std::string& json)
        : native_name{native}, json_name{json} {}

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

/*! Base class that satisfies the requirements from rapidjson */
class RapidJsonDeserializerBase {
public:
    enum class State { INIT, IN_OBJECT, IN_ARRAY, RECURSED, DONE };

    static std::string to_string(const State& state) {
        const static std::array<std::string, 5> states =
            {"INIT", "IN_OBJECT", "IN_ARRAY", "RECURSED", "DONE"};
        return states.at(static_cast<int>(state));
    }

    RapidJsonDeserializerBase(RapidJsonDeserializerBase *parent)
    : parent_{parent} {}

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


struct SerializeProperties {
    SerializeProperties()
    : max_memory_consumption{GetDefaultMaxMemoryConsumption()}
    , ignore_empty_fileds{true}
    , ignore_unknown_properties{true}
    {}

    explicit SerializeProperties(const bool ignoreEmptyFields)
    : max_memory_consumption{GetDefaultMaxMemoryConsumption()}
    , ignore_empty_fileds{ignoreEmptyFields}
    , ignore_unknown_properties{true}
    {}

    uint64_t max_memory_consumption : 32;
    uint64_t ignore_empty_fileds : 1;
    uint64_t ignore_unknown_properties : 1;

    const std::set<std::string> *excluded_names = nullptr;
    const JsonFieldMapping *name_mapping = nullptr;

    constexpr static uint64_t GetDefaultMaxMemoryConsumption() { return 1024 * 1024; }

    bool is_excluded(const std::string& name) const noexcept {
        return excluded_names
            && excluded_names->find(name) != excluded_names->end();
    }

    const std::string& map_name_to_json(const std::string& name) const noexcept {
        if (name_mapping == nullptr)
            return name;
        return name_mapping->to_json_name(name);
    }

    int64_t GetMaxMemoryConsumption() const noexcept {
        return static_cast<int64_t>(max_memory_consumption);
    }

    void SetMaxMemoryConsumption(std::int64_t val) {
        if ((val < 0) || (val > 0xffffffffL)) {
            throw ConstraintException("Memory contraint value is out of limit");
        }
        max_memory_consumption = static_cast<uint32_t>(val);
    }
};

using serialize_properties_t = SerializeProperties;

namespace {

template <typename T>
struct type_conv
{
    using const_field_type_t = const T;
    using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
    using noconst_ref_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;
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

template <typename T>
struct get_len {
    size_t operator()(const T& v) const {
        return sizeof(v);
    }
};

template <>
struct get_len<const std::string&> {
    size_t operator()(const std::string& v) const {
        const auto ptrlen = sizeof(const char *);
        size_t b = sizeof(v);
        if (v.size() > (sizeof(std::string) + ptrlen)) {
            b += v.size() - ptrlen;
        }
        return b;
    }
};

template <>
struct get_len<std::string&> {
    size_t operator()(const std::string& v) const {
        assert(false); //oops!
        return get_len<const std::string&>()(v);
    }
};


#if !defined(_MSC_VER) || (_MSC_VER >= 1910) // g++, clang, Visual Studio from 2017

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
	RESTC_CPP_LOG_ERROR << "assign_value: Invalid data conversion from "
		<< RESTC_CPP_TYPENAME(varT) << " to " << RESTC_CPP_TYPENAME(valT);
    assert(false);
	throw ParseException("assign_value: Invalid data conversion from ");
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
struct is_map {
    constexpr static const bool value = false;
};

template <typename T, typename CompareT, typename AllocT>
struct is_map<std::map<std::string, T, CompareT, AllocT> > {
    constexpr static const bool value = true;
};


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

class RapidJsonSkipObject :  public RapidJsonDeserializerBase {
public:

    RapidJsonSkipObject(RapidJsonDeserializerBase *parent)
    : RapidJsonDeserializerBase(parent)
    {
    }

    bool Null() override {
        return true;
    }

    bool Bool(bool b) override {
        return true;
    }

    bool Int(int) override {
        return true;
    }

    bool Uint(unsigned) override {
        return true;
    }

    bool Int64(int64_t) override {
        return true;
    }

    bool Uint64(uint64_t) override {
        return true;
    }

    bool Double(double) override {
        return true;
    }

    bool String(const char*, std::size_t Tlength, bool) override {
        return true;
    }

    bool RawNumber(const char*, std::size_t, bool) override {
        return true;
    }

    bool StartObject() override {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << "   Skipping json: StartObject()";
#endif
        ++recursion_;
        return true;
    }

    bool Key(const char* str, std::size_t length, bool copy) override {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << "   Skipping json key: "
            << boost::string_ref(str, length);
#endif
        return true;
    }

    bool EndObject(std::size_t memberCount) override {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << "   Skipping json: EndObject()";
#endif
        if (--recursion_ <= 0) {
            if (HaveParent()) {
                GetParent().OnChildIsDone();
            }
        }
        return true;
    }

    bool StartArray() override {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << "   Skipping json: StartArray()";
#endif
        ++recursion_;
        return true;
    }

    bool EndArray(std::size_t elementCount) override {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << "   Skipping json: EndArray()";
#endif
        if (--recursion_ <= 0) {
            if (HaveParent()) {
                GetParent().OnChildIsDone();
            }
        }
        return true;
    }

private:
    int recursion_ = 0;
};

} // anonymous namespace

template <typename T>
class RapidJsonDeserializer : public RapidJsonDeserializerBase {
public:
    using data_t = typename std::remove_const<typename std::remove_reference<T>::type>::type;
    static constexpr int const default_mem_limit = 1024 * 1024 * 1024;

    RapidJsonDeserializer(data_t& object)
    : RapidJsonDeserializerBase(nullptr)
    , object_{object}
    , properties_buffer_{std::make_unique<serialize_properties_t>()}
    , properties_{*properties_buffer_}
    , bytes_buffer_{properties_.GetMaxMemoryConsumption()}
    , bytes_{bytes_buffer_ ? &bytes_buffer_ : nullptr}
    {}

    explicit RapidJsonDeserializer(data_t& object, const serialize_properties_t& properties)
    : RapidJsonDeserializerBase(nullptr)
    , object_{object}
    , properties_{properties}
    , bytes_buffer_{properties_.GetMaxMemoryConsumption()}
    , bytes_{bytes_buffer_ ? &bytes_buffer_ : nullptr}
    {}

    explicit RapidJsonDeserializer(data_t& object,
                        RapidJsonDeserializerBase *parent,
                        const serialize_properties_t& properties,
                        std::int64_t *maxBytes)
    : RapidJsonDeserializerBase(parent)
    , object_{object}
    , properties_{properties}
    , bytes_{maxBytes}
    {
        assert(parent != nullptr);
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
            || is_container<classT>::value || is_map<classT>::value
            >::type* = 0) {

        using const_field_type_t = decltype(item);
        using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
        using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

        auto& value = const_cast<field_type_t&>(item);

        recursed_to_ = std::make_unique<RapidJsonDeserializer<field_type_t>>(
            value, this, properties_, bytes_);
    }

    template <typename classT, typename itemT>
    void DoRecurseToMember(itemT& field,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<classT>::value
            && !is_container<classT>::value
            && !is_map<classT>::value
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
        recursed_to_ = std::make_unique<RapidJsonDeserializer<native_type_t>>(
            object_.back(), this, properties_, bytes_);
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

        if (!recursed_to_) {
            assert(!found);
            RESTC_CPP_LOG_DEBUG << "RecurseToMember(): Failed to find property-name '"
                << current_name_
                << "' in C++ class '" << RESTC_CPP_TYPENAME(dataT)
                << "' when serializing from JSON.";

            if (!properties_.ignore_unknown_properties) {
                throw UnknownPropertyException(current_name_);
            } else {
                recursed_to_ = std::make_unique<RapidJsonSkipObject>(this);
            }
        } else {
            assert(found);
        }

        assert(recursed_to_);
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

                this->AddBytes(get_len<decltype(new_value_)>()(new_value_));
                assign_value<decltype(value), decltype(new_value_)>(value, new_value_);
                found = true;
            }
        };

        on_name_and_value<dataT, decltype(fn)> handler(fn);
        handler.for_each_member(object_);

        if (!found) {
            assert(!found);
            RESTC_CPP_LOG_DEBUG << "SetValueOnMember(): Failed to find property-name '"
                << current_name_
                << "' in C++ class '" << RESTC_CPP_TYPENAME(dataT)
                << "' when serializing from JSON.";

            if (!properties_.ignore_unknown_properties) {
                throw UnknownPropertyException(current_name_);
            }
        }

        current_name_.clear();
        return true;
    }

    // std::map / the key must be a string.
    template<typename dataT, typename argT>
    bool SetValueOnMember(const argT& val,
        typename std::enable_if<
            is_map<dataT>::value
            >::type* = 0) {
        assert(!current_name_.empty());

        AddBytes(
            get_len<argT>()(val)
            + sizeof(std::string)
            + current_name_.size()
            + sizeof(size_t) * 6 // FIXME: Find approximate average overhead for map
        );

		assign_value(object_[current_name_], val);
        current_name_.clear();
        return true;
    }

    template<typename dataT, typename argT>
    bool SetValueOnMember(argT val,
        typename std::enable_if<
            !boost::fusion::traits::is_sequence<dataT>::value
            && !is_map<dataT>::value
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


        AddBytes(
            get_len<argT>()(val)
            + sizeof(size_t) * 3 // Approximate average overhead for container
        );

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
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " SetValue: " << current_name_
            << " State: " << to_string(state_);
#endif
        if (state_ == State::IN_OBJECT) {
            return SetValueOnMember<data_t>(val);
        } else if (state_ == State::IN_ARRAY) {
            SetValueInArray<data_t>(val);
            return true;
        }
        RESTC_CPP_LOG_ERROR << RESTC_CPP_TYPENAME(data_t)
            << " SetValue: " << current_name_
            << " State: " << to_string(state_)
            << " Value type" << RESTC_CPP_TYPENAME(argT);

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
        return false;
    }

    bool DoStartObject() {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        const bool i_am_a_map = is_map<data_t>::value;
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoStartObject: " << current_name_
            << " i_am_a_map: " << i_am_a_map;
#endif
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

                // If this fails, you probably need to declare the data-type in the array.
                // with BOOST_FUSION_ADAPT_STRUCT or friends.
                assert(recursed_to_);
                recursed_to_->StartObject();
                break;
            case State::DONE:
                RESTC_CPP_LOG_TRACE << "Re-using instance of RapidJsonDeserializer";
                state_ = State::IN_OBJECT;
                if (bytes_) {
                    *bytes_ = properties_.GetMaxMemoryConsumption();
                }
                break;
            default:
                assert(false && "Unexpected state");

        }
        return true;
    }

    bool DoKey(const char* str, std::size_t length, bool copy) {
        assert(current_name_.empty());

        if (properties_.name_mapping == nullptr) {
            current_name_.assign(str, length);
        } else {
            std::string name{str, length};
            current_name_ = properties_.name_mapping->to_native_name(name);
        }
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoKey: " << current_name_;
#endif
        return true;
    }

    bool DoEndObject(std::size_t memberCount) {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoEndObject: " << current_name_;
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
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoStartArray: " << current_name_;
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
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " DoEndArray: " << current_name_;
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
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(data_t)
            << " OnChildIsDone";
#endif
        assert(state_ == State::RECURSED);
        assert(!saved_state_.empty());

        state_ = saved_state_.top();
        saved_state_.pop();
        recursed_to_.reset();
    }

    void AddBytes(size_t bytes) {
        if (!bytes_) {
            return;
        }
        static const std::string oom{"Exceed memory usage constraint"};
        *bytes_ -= bytes;
        if (*bytes_ <= 0) {
            throw ConstraintException(oom);
        }
    }

private:
    data_t& object_;

    // The root objects owns the counter.
    // Child objects gets the bytes_ pointer in the constructor.
    std::unique_ptr<serialize_properties_t> properties_buffer_;
    const serialize_properties_t& properties_;
    std::int64_t bytes_buffer_ = {};
    std::int64_t *bytes_ = &bytes_buffer_;

    //const JsonFieldMapping *name_mapping_ = nullptr;
    std::string current_name_;
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
                const serialize_properties_t& properties,
    typename std::enable_if<
        !boost::fusion::traits::is_sequence<dataT>::value
        && !std::is_integral<dataT>::value
        && !std::is_floating_point<dataT>::value
        && !std::is_same<dataT, std::string>::value
        && !is_container<dataT>::value
        && !is_map<dataT>::value
        >::type* = 0) {
    assert(false);
};

template <typename dataT, typename serializerT>
void do_serialize(const dataT& object, serializerT& serializer,
                const serialize_properties_t& properties,
    typename std::enable_if<
        std::is_integral<dataT>::value
        || std::is_floating_point<dataT>::value
        >::type* = 0) {

    do_serialize_integral(object, serializer);
};

template <typename dataT, typename serializerT>
void do_serialize(const dataT& object, serializerT& serializer,
                const serialize_properties_t& properties,
    typename std::enable_if<
        std::is_same<dataT, std::string>::value
        >::type* = 0) {

    serializer.String(object.c_str(),
        static_cast<rapidjson::SizeType>(object.size()),
        true);
};

template <typename dataT, typename serializerT>
void do_serialize(const dataT& object, serializerT& serializer,
                const serialize_properties_t& properties,
    typename std::enable_if<
        boost::fusion::traits::is_sequence<dataT>::value
        >::type* = 0);

template <typename dataT, typename serializerT>
void do_serialize(const dataT& object, serializerT& serializer,
                const serialize_properties_t& properties,
    typename std::enable_if<
        is_container<dataT>::value
        >::type* = 0) {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
    RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
        << " StartArray: ";
#endif
    serializer.StartArray();

    for(const auto& v: object) {

        using native_field_type_t = typename std::remove_const<
            typename std::remove_reference<decltype(v)>::type>::type;

        do_serialize<native_field_type_t>(v, serializer, properties);
    }
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
    RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
        << " EndArray: ";
#endif
    serializer.EndArray();
};

template <typename dataT, typename serializerT>
void do_serialize(const dataT& object, serializerT& serializer,
                const serialize_properties_t& properties,
     typename std::enable_if<
        is_map<dataT>::value
        >::type* = 0) {

    static const serialize_properties_t map_name_properties{false};

#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
    RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
        << " StartMap: ";
#endif
    serializer.StartObject();

    for(const auto& v: object) {

        using native_field_type_t = typename std::remove_const<
            typename std::remove_reference<typename dataT::mapped_type>::type>::type;

        do_serialize<std::string>(v.first, serializer, map_name_properties);
        do_serialize<native_field_type_t>(v.second, serializer, properties);
    }
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
    RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
        << " EndMap: ";
#endif
    serializer.EndObject();
};

template <typename dataT, typename serializerT>
void do_serialize(const dataT& object, serializerT& serializer,
                const serialize_properties_t& properties,
    typename std::enable_if<
        boost::fusion::traits::is_sequence<dataT>::value
        >::type*) {

    serializer.StartObject();
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
    RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
        << " StartObject: ";
#endif
    auto fn = [&](const char *name, auto& val) {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
            << " Key: " << name;
#endif
        if (properties.ignore_empty_fileds) {
            if (is_empty_field(val)) {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
        RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
            << " ignoring empty field.";
#endif
                return;
            }
        }

        if (properties.excluded_names
            && properties.is_excluded(name)) {
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
            RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
                << " ignoring excluded field.";
#endif
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
#ifdef RESTC_CPP_LOG_JSON_SERIALIZATION
    RESTC_CPP_LOG_TRACE << RESTC_CPP_TYPENAME(dataT)
        << " EndObject: ";
#endif
    serializer.EndObject();
};
} // namespace

/*! Recursively serialize the C++ object to the json serializer */
template <typename objectT, typename serializerT>
class RapidJsonSerializer
{
public:
    using data_t = typename std::remove_const<typename std::remove_reference<objectT>::type>::type;

    RapidJsonSerializer(const data_t& object, serializerT& serializer)
    : object_{object}, serializer_{serializer}
    {
    }

    // See https://github.com/miloyip/rapidjson/blob/master/doc/sax.md#writer-writer
    void Serialize() {
        do_serialize<data_t>(object_, serializer_, properties_);
    }

    void IgnoreEmptyMembers(bool ignore = true) {
        properties_.ignore_empty_fileds = ignore;
    }

    // Set to nullptr to disable lookup
    void ExcludeNames(excluded_names_t *names) {
        properties_.excluded_names = names;
    }

    void SetNameMapping(const JsonFieldMapping *mapping) {
        properties_.name_mapping = mapping;
    }

private:

    const data_t& object_;
    serializerT& serializer_;
    serialize_properties_t properties_;
};

/*! Serialize an object or a list of objects of type T to the wire
 *
*/
template <typename T>
class RapidJsonInserter
{
public:
    using stream_t = RapidJsonWriter<char>;
    using writer_t = rapidjson::Writer<stream_t>;

    /*! Constructor
     *
     * \param writer Output DataWriter
     * \param isList True if we want to serialize a Json list of objects.
     *      If false, we can only Add() one object.
     */
    RapidJsonInserter(DataWriter& writer, bool isList = false)
    : is_list_{isList}, stream_{writer}, writer_{stream_} {}

    ~RapidJsonInserter() {
        Done();
    }

    /*! Serialize one object
     *
     * If the constructor was called with isList = false,
     * it can only be called once.
     */
    void Add(const T& v) {
        if (state_ == State::DONE) {
            throw RestcCppException("Object is DONE. Cannot Add more data.");
        }

        if (state_ == State::PRE) {
            if (is_list_) {
                writer_.StartArray();
            }
            state_ = State::ITERATING;
        }

        do_serialize<T>(v, writer_, properties_);
    }

    /*! Mark the serialization as complete */
    void Done() {
        if (state_ == State::ITERATING) {
            if (is_list_) {
                writer_.EndArray();
            }
        }
        state_ = State::DONE;
    }

    void IgnoreEmptyMembers(bool ignore = true) {
        properties_.ignore_empty_fileds = ignore;
    }

    // Set to nullptr to disable lookup
    void ExcludeNames(const excluded_names_t *names) {
        properties_.excluded_names = names;
    }

    void SetNameMapping(const JsonFieldMapping *mapping) {
        properties_.name_mapping = mapping;
    }

private:
    enum class State { PRE, ITERATING, DONE };

    State state_ = State::PRE;
    const bool is_list_;
    stream_t stream_;
    writer_t writer_;
    serialize_properties_t properties_;
};

/*! Serialize a std::istream to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData,
    std::istream& stream, const serialize_properties_t& properties) {

    RapidJsonDeserializer<dataT> handler(rootData, properties);
    rapidjson::IStreamWrapper input_stream_reader(stream);
    rapidjson::Reader json_reader;
    json_reader.Parse(input_stream_reader, handler);
}

/*! Serialize a std::istream to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData,
    std::istream& stream) {

    serialize_properties_t properties;
    SerializeFromJson(rootData, stream, properties);
}

/*! Serialize a reply to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData, Reply& reply,
                       const serialize_properties_t& properties) {

    RapidJsonDeserializer<dataT> handler(rootData, properties);
    RapidJsonReader reply_stream(reply);
    rapidjson::Reader json_reader;
    json_reader.Parse(reply_stream, handler);
}

/*! Serialize a reply to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData, Reply& reply) {
    serialize_properties_t properties;
    SerializeFromJson(rootData, reply, properties);
}

/*! Serialize a reply to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData, std::unique_ptr<Reply>&& reply) {
    serialize_properties_t properties;
    SerializeFromJson(rootData, *reply, properties);
}

/*! Serialize a reply to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData,
    Reply& reply,
    const JsonFieldMapping *nameMapper,
    std::int64_t maxBytes = RapidJsonDeserializer<dataT>::default_mem_limit) {

    serialize_properties_t properties;
    properties.max_memory_consumption = maxBytes;
    properties.name_mapping = nameMapper;

    SerializeFromJson(rootData, reply, properties);
}

/*! Serialize a reply to a C++ class instance */
template <typename dataT>
void SerializeFromJson(dataT& rootData,
    std::unique_ptr<Reply>&& reply,
    const JsonFieldMapping *nameMapper,
    std::int64_t maxBytes = RapidJsonDeserializer<dataT>::default_mem_limit) {

    serialize_properties_t properties;
    properties.SetMaxMemoryConsumption(maxBytes);
    properties.name_mapping = nameMapper;

    SerializeFromJson(rootData, *reply, properties);
}

} // namespace

