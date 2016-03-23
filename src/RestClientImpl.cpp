

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"

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
        boost::asio::yield_context& GetYield() { return yield_; }

        std::unique_ptr<Reply> Get(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::GET, rc_);
            return Request(*req);
        }

        std::unique_ptr<Reply> Request(restc_cpp::Request& req) override {
            return req.Execute(*this);
        }

        boost::asio::yield_context& yield_;
        RestClientImpl& rc_;
    };

    RestClientImpl(Request::Properties *properties)
    : default_connection_properties_{make_shared<Request::Properties>()}
    {
        if (properties)
            *default_connection_properties_ = *properties;

        std::thread([this]() {
            pool_ = ConnectionPool::Create(*this);
            work_ = make_unique<boost::asio::io_service::work>(io_service_);
            io_service_.run();
        }).detach();
    }

    const Request::Properties::ptr_t GetConnectionProperties() const override {
        return default_connection_properties_;
    }


    const void ProcessInWorker(boost::asio::yield_context yield,
                               const prc_fn_t& fn) {

        ContextImpl ctx(yield, *this);

        try {
            fn(ctx);
        } catch(std::exception& ex) {
            std::ostringstream msg;
            msg << "Caught exception: " << ex.what();
            RestClient::LogError(msg);
        }
    }

    void Process(const prc_fn_t& fn) override {
        boost::asio::spawn(io_service_,
                           bind(&RestClientImpl::ProcessInWorker, this,
                                std::placeholders::_1, fn));
    }

    ConnectionPool& GetConnectionPool() { return *pool_; }

    boost::asio::io_service& GetIoService() { return io_service_; }

    void LogError(const char *message) override {
        log_error_(message);
    }

    void LogWarning(const char *message) override {
        log_warn_(message);
    }

    void LogNotice(const char *message) override {
        log_notice_(message);
    }

    void LogDebug(const char *message) override {
        log_debug_(message);
    }


private:
    Request::Properties::ptr_t default_connection_properties_;
    boost::asio::io_service io_service_;
    unique_ptr<boost::asio::io_service::work> work_;
    unique_ptr<ConnectionPool> pool_;

    logger_t log_error_ = [](const char *msg) { std::clog << "ERROR: " << msg << std::endl; };
    logger_t log_warn_ = [](const char *msg) { std::clog << "WARN: " << msg << std::endl; };
    logger_t log_notice_ = [](const char *msg) { std::clog << "NOTICE: " << msg << std::endl; };
    logger_t log_debug_ = [](const char *msg) { std::clog << "DEBUG: " << msg << std::endl; };

};

unique_ptr<RestClient> RestClient::Create(Request::Properties *properties) {
    return make_unique<RestClientImpl>(properties);
}

} // restc_cpp
