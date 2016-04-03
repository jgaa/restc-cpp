#pragma once

#include <iostream>
#include <type_traits>
#include <assert.h>
#include <stack>
#include <boost/type_index.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/zip.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>

namespace restc_cpp {

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


} // anonymous namespace

template <typename T>
class RapidJsonHandlerImpl : public RapidJsonHandler {
public:
    using data_t = T;
    enum class State { INIT, IN_OBJECT, RECURSED, DONE };


    RapidJsonHandlerImpl(data_t& object, RapidJsonHandler *parent = nullptr)
    : RapidJsonHandler(parent)
    , object_{object}
    , struct_members_{with_names(object_)}
    {

    }

    bool Null() override {
        return Route().DoNull();
    }

    bool Bool(bool b) override {
        return Route().DoBool(b);
    }

    bool Int(int i) override {
        return Route().DoInt(i);
    }

    bool Uint(unsigned u) override {
        return Route().DoUint(u);
    }

    bool Int64(int64_t i) override {
        return Route().DoInt64(i);
    }

    bool Uint64(uint64_t u) override {
        return Route().DoUint64(u);
    }

    bool Double(double d) override {
        return Route().DoDouble(d);
    }

    bool String(const char* str, std::size_t length, bool copy) override {
        return Route().DoString(str, length, copy);
    }

    bool RawNumber(const char* str, std::size_t length, bool copy) override {
        return Route().DoRawNumber(str, length, copy);
    }

    bool StartObject() override {
        return Route().DoStartObject();
    }

    bool Key(const char* str, std::size_t length, bool copy) override {
        return Route().DoKey(str, length, copy);
    }

    bool EndObject(std::size_t memberCount) override {
        return Route().DoEndObject(memberCount);
    }

    bool StartArray() override {
        return Route().DoStartArray();
    }

    bool EndArray(std::size_t elementCount) override {
        return Route().DoEndArray(elementCount);
    }

private:

    template <typename classT, typename itemT>
    void DoRecurseToMember(itemT& item,
                           typename std::enable_if<boost::fusion::traits::is_sequence<classT>::value>::type* = 0) {

        using const_field_type_t = decltype(get_value(item));
        using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
        using field_type_t = typename std::add_lvalue_reference<native_field_type_t>::type;

        auto& const_value = get_value(item);
        auto& value = const_cast<field_type_t&>(const_value);

        recursed_to_ = std::make_unique<RapidJsonHandlerImpl<field_type_t>>(value, this);
    }

    template <typename classT, typename itemT>
    void DoRecurseToMember(itemT& field, typename std::enable_if<!boost::fusion::traits::is_sequence<classT>::value>::type* = 0) {
        assert(false);
    }


    bool RecurseToMember() {
        assert(!recursed_to_);
        assert(!current_name_.empty());

        bool found = false;
        boost::fusion::for_each(struct_members_, [&](const auto item) {
            /* It's probably better to use a recursive search,
             * but this will do for now.
             */
            if (found) {
                return;
            }

            if (get_name(item).compare(current_name_) == 0) {
                using const_field_type_t = decltype(boost::fusion::at_c<0>(item));
                using native_field_type_t = typename std::remove_const<typename std::remove_reference<const_field_type_t>::type>::type;
                DoRecurseToMember<native_field_type_t>(item);
                found = true;
            }
        });

        assert(recursed_to_);
        assert(found);
        saved_state_.push(state_);
        state_ = State::RECURSED;
        current_name_.clear();
        return recursed_to_->StartObject();
    }

    template<typename argT>
    bool SetValueOnMember(argT val) {
        assert(!current_name_.empty());

        bool found = false;
        boost::fusion::for_each(struct_members_, [&](const auto item) {
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

    template<typename argT>
    bool SetValue(argT val) {
        if (state_ == State::IN_OBJECT) {
            return SetValueOnMember(val);
        }
        assert(false && "Invalid state for setting a value");
    }

    bool DoNull() override {
        // TODO: Clear value
        return true;
    }

    bool DoBool(bool b) override {
        return SetValue(b);
    }

    bool DoInt(int i) override {
        return SetValue(i);
    }

    bool DoUint(unsigned u) override {
        return SetValue(u);
    }

    bool DoInt64(int64_t i) override {
        return SetValue(i);
    }

    bool DoUint64(uint64_t u) override {
        return SetValue(u);
    }

    bool DoDouble(double d) override {
        return SetValue(d);
    }

    bool DoString(const char* str, std::size_t length, bool copy) override {
        return SetValue(std::string(str, length));
    }

    bool DoRawNumber(const char* str, std::size_t length, bool copy) override {
        assert(false);
    }

    bool DoStartObject() override {
        // TODO: Recurse into nested objects
        if (state_ == State::INIT) {
            state_ = State::IN_OBJECT;
        } else if (state_ == State::IN_OBJECT) {
            return RecurseToMember();
        }
        return true;
    }

    bool DoKey(const char* str, std::size_t length, bool copy) override {
        assert(current_name_.empty());
        current_name_.assign(str, length);
        return true;
    }

    bool DoEndObject(std::size_t memberCount) override {
        current_name_.clear();
        if (state_ == State::IN_OBJECT) {
            if (HaveParent()) {
                GetParent().OnChildIsDone();
            }
        } else {
            state_ = State::DONE;
        }
        return true;
    }

    bool DoStartArray() override {
        // TODO: Recurse into nested objects
        return true;
    }

    bool DoEndArray(std::size_t elementCount) override {
        return true;
    }

    void OnChildIsDone() override {
        assert(state_ == State::RECURSED);
        assert(!saved_state_.empty());

        state_ = saved_state_.top();
        saved_state_.pop();
        recursed_to_.reset();
    }

    RapidJsonHandler& Route() {
        if (state_ == State::RECURSED) {
            assert(recursed_to_);
            return *recursed_to_;
        }
        assert(recursed_to_ == nullptr);
        return *this;
    }

private:
    data_t& object_;
    std::string current_name_;
    decltype(with_names(object_)) struct_members_;
    State state_ = State::INIT;
    std::stack<State> saved_state_;
    std::unique_ptr<RapidJsonHandler> recursed_to_;
};



} // namespace

