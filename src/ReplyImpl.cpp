#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"

using namespace std;

namespace restc_cpp {

class ReplyImpl : public Reply {
public:

    ReplyImpl(Context& ctx,
              const Request& req,
              RestClient& owner)
    : ctx_{ctx}, req_{req}, owner_{owner}
    {
    }

private:
    Context& ctx_;
    const Request& req_;
    RestClient& owner_;
};


std::unique_ptr<Reply>
Reply::Create(Context& ctx,
       const Request& req,
       RestClient& owner) {

    return make_unique<ReplyImpl>(ctx, req, owner);
}

} // restc_cpp

