#include <cassert>
#include <array>

#include <boost/utility/string_ref.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/RequestBody.h"
#include "restc-cpp/DataWriter.h"

using namespace std;


namespace restc_cpp {
namespace impl {

class RequestBodyStringImpl : public RequestBody
{
public:
    RequestBodyStringImpl(string body)
    : body_{move(body)}
    {
    }

    Type GetType() const noexcept override {
        return Type::FIXED_SIZE;
    }

    std::uint64_t GetFixedSize() const override {
        return body_.size();
    }

    bool GetData(write_buffers_t & buffers) override {
        if (eof_) {
            return false;
        }

        buffers.push_back({body_.c_str(), body_.size()});
        eof_ = true;
        return true;
    }

    void Reset() override {
        eof_ = false;
    }

    std::string  GetCopyOfData() const override {
        return body_;
    }


private:
    string body_;
    bool eof_ = false;
};


} // impl

std::unique_ptr<RequestBody> RequestBody::CreateStringBody(
    std::string body) {

    return make_unique<impl::RequestBodyStringImpl>(move(body));
}

} // restc_cpp

