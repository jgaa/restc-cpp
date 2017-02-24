
#pragma once

#include "restc-cpp/DataWriter.h"

namespace restc_cpp {

/*! Wrapper from rapidjson (output) Stream concept to our DataWriter
 */

template <typename chT = char, size_t bufferSizeT = 1024 * 16>
class RapidJsonWriter {
public:
    using Ch = chT;

    RapidJsonWriter(DataWriter& writer)
    : writer_{writer} {}

    Ch Peek() const { assert(false); return '\0'; }

    Ch Take() { assert(false); return '\0'; }

    size_t Tell() const { return 0; }

    Ch* PutBegin() { assert(false); return 0; }

    void Put(Ch c) {
        buffer_[bytes_] = c;
        if (++bytes_ == buffer_.size()) {
            Flush();
        }
    }

    void Flush() {
        if (bytes_ == 0) {
            return;
        }

        writer_.Write({buffer_.data(), bytes_ * sizeof(Ch)});
        bytes_ = 0;
    }

    size_t PutEnd(Ch*) { assert(false); return 0; }

private:
    DataWriter& writer_;
    std::array<Ch, bufferSizeT> buffer_;
    size_t bytes_ = 0;
};


} // namespace

