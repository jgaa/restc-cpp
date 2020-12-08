#pragma once

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/SerializeJson.h"

namespace restc_cpp {


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
        const serialize_properties_t *properties = nullptr,
        bool withoutLeadInSquereBrancket = false)
    : reply_stream_{reply}, properties_{properties}
    {
        if (!properties_) {
            pbuf = std::make_unique<serialize_properties_t>();
            properties_ = pbuf.get();
        }
        if (withoutLeadInSquereBrancket) {
            state_ = State::ITERATING;
        }
    }

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
                throw ParseException("Expected leading json '['");
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
                        *data, *properties_);
                    json_reader_.Parse(reply_stream_, handler);
                    return move(data);
                } else if (ch == ']') {
                    reply_stream_.Take();
                    state_ = State::DONE;
                    break;
                } else {
                    static const std::string err_msg{"IteratorFromJsonSerializer: Unexpected character in input stream: "};
                    throw ParseException(err_msg + std::string(1, ch));
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
    std::unique_ptr<serialize_properties_t> pbuf;
    const serialize_properties_t *properties_ = nullptr;
};

} // namespace

