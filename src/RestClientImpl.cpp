

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"

using namespace std;

namespace restc_cpp {


class  RestClientImpl : public RestClient {
public:

    class ContextImpl : public Context {
    public:
        ContextImpl(boost::asio::yield_context& yield,
                    RestClientImpl& rc)
        : yield_{yield}
        , rc_{rc}
        {}

        RestClient& GetClient() override { return rc_; }
        boost::asio::yield_context& GetYield() override { return yield_; }

        std::unique_ptr<Reply> Get(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::GET, rc_);
            return Request(*req);
        }

        unique_ptr< Reply > Post(string url, string body) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::POST, rc_,
                                       {make_unique<Request::Body>(move(body))});
            return Request(*req);
        }

        unique_ptr< Reply > Put(string url, string body) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::PUT, rc_,
                                       {make_unique<Request::Body>(move(body))});
            return Request(*req);
        }

        unique_ptr< Reply > Delete(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::DELETE, rc_);
            return Request(*req);
        }

        std::unique_ptr<Reply> Request(restc_cpp::Request& req) override {
            return req.Execute(*this);
        }

        boost::asio::yield_context& yield_;
        RestClientImpl& rc_;
    };

    RestClientImpl(boost::optional<Request::Properties> properties)
    : default_connection_properties_{make_shared<Request::Properties>()}
    {
        static const string content_type{"Content-Type"};
        static const string json_type{"application/json; charset=utf-8"};

        if (properties)
            *default_connection_properties_ = *properties;

        if (default_connection_properties_->headers.find(content_type)
            == default_connection_properties_->headers.end()) {
            default_connection_properties_->headers[content_type] = json_type;
        }

        std::promise<void> wait;
        auto done = wait.get_future();

        std::thread([&]() {
            pool_ = ConnectionPool::Create(*this);
            work_ = make_unique<boost::asio::io_service::work>(io_service_);
            wait.set_value();
            io_service_.run();
        }).detach();

        // Wait for the ConnectionPool to be constructed
        done.get();
    }

    const Request::Properties::ptr_t GetConnectionProperties() const override {
        return default_connection_properties_;
    }


    const void
    ProcessInWorker(boost::asio::yield_context yield,
                    const prc_fn_t& fn,
                    const std::shared_ptr<std::promise<void>>& promise) {

        ContextImpl ctx(yield, *this);

        try {
            fn(ctx);
        } catch(std::exception& ex) {
            RESTC_CPP_LOG_ERROR << "Caught exception: " << ex.what();
            if (promise) {
                promise->set_exception(std::current_exception());
            }
            return;
        }

        if (promise) {
            promise->set_value();
        }
    }

    void Process(const prc_fn_t& fn) override {
        boost::asio::spawn(io_service_,
                           bind(&RestClientImpl::ProcessInWorker, this,
                                std::placeholders::_1, fn, nullptr));
    }

    std::future< void > ProcessWithPromise(const prc_fn_t& fn) override {
        auto promise = make_shared<std::promise<void>>();
        auto future = promise->get_future();

        boost::asio::spawn(io_service_,
                           bind(&RestClientImpl::ProcessInWorker, this,
                                std::placeholders::_1, fn, promise));

        return future;
    }

    ConnectionPool& GetConnectionPool() override { return *pool_; }

    boost::asio::io_service& GetIoService() override { return io_service_; }

private:
    Request::Properties::ptr_t default_connection_properties_;
    boost::asio::io_service io_service_;
    unique_ptr<boost::asio::io_service::work> work_;
    unique_ptr<ConnectionPool> pool_;
};

unique_ptr<RestClient> RestClient::Create(boost::optional<Request::Properties> properties) {
    return make_unique<RestClientImpl>(properties);
}

} // restc_cpp
