#pragma once

#ifndef RESTC_CPP_RAPID_JSON_READER_H_
#define RESTC_CPP_RAPID_JSON_READER_H_


#include <assert.h>

#include "rapidjson/reader.h"
#include "restc-cpp/restc-cpp.h"

namespace restc_cpp {


/*! Rapidjson input stream implementation
 *
 * Ref: https://github.com/miloyip/rapidjson/blob/master/doc/stream.md
 */
class RapidJsonReader {
public:

    RapidJsonReader(Reply& reply)
    : reply_{reply}
    {
        Read();
    }

    using Ch = char;

    //! Read the current character from stream without moving the read cursor.
    Ch Peek() const noexcept {
        if (IsEof()) {
            return 0; // EOF
        }

        assert(ch_ && (ch_ != end_));
        return *ch_;
    }

    //! Read the current character from stream and moving the read cursor to next character.
    Ch Take() {
        if (IsEof()) {
            return 0; // EOF
        }

        assert(ch_ && (ch_ != end_));
        const auto ch = *ch_;
        ++pos_;

        if (++ch_ == end_) {
            Read();
        }

        return ch;
    }

    //! Get the current read cursor.
    //! \return Number of characters read from start.
    size_t Tell() noexcept {
        return pos_;
    }

    bool IsEof() const noexcept {
        return ch_ == nullptr;
    }

    //! Begin writing operation at the current read pointer.
    //! \return The begin writer pointer.
    Ch* PutBegin() {
        assert(false);
        return nullptr;
    }

    //! Write a character.
    void Put(Ch c) {
        assert(false);
    }

    //! Flush the buffer.
    void Flush() {
        assert (false);
    }

    //! End the writing operation.
    //! \param begin The begin write pointer returned by PutBegin().
    //! \return Number of characters written.
    size_t PutEnd(Ch* begin) {
        assert(false);
        return 0;
    }

private:
    void Read() {
        const auto buffer = reply_.GetSomeData();

        const auto len = boost::asio::buffer_size(buffer);

        if (len) {
            ch_ = boost::asio::buffer_cast<const char*>(buffer);
            end_ = ch_ + len;
        } else {
            ch_ = end_ = nullptr;
        }
    }

    const char *ch_ = nullptr;
    const char *end_ = nullptr;
    size_t pos_ = 0;
    Reply& reply_;
};



} // restc_cpp

#endif //RESTC_CPP_RAPID_JSON_READER_H_

