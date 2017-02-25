#pragma once

#ifndef RESTC_CPP_BODY_WRITER_H_
#define RESTC_CPP_BODY_WRITER_H_

#include "restc-cpp/DataWriter.h"
#include "restc-cpp/RequestBody.h"
#include "restc-cpp/error.h"

namespace restc_cpp {

/*! The body of the request. */
template <typename fnT>
class RequestBodyWriter : public RequestBody {
public:
    RequestBodyWriter(fnT fn)
    : fn_{std::move(fn)}
    {}

    Type GetType() const noexcept override {
        return Type::CHUNKED_LAZY_PUSH;
    };

    void PushData(DataWriter& writer) override {
        fn_(writer);
    }

private:
    fnT fn_;
};

} // restc_cpp

#endif // RESTC_CPP_BODY_WRITER_H_
