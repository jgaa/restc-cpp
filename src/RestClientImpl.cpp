

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"
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
                                       {Request::Body{move(body)}});
            return Request(*req);
        }

        unique_ptr< Reply > Put(string url, string body) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::PUT, rc_,
                                       {Request::Body{move(body)}});
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

    ConnectionPool& GetConnectionPool() override { return *pool_; }

    boost::asio::io_service& GetIoService() override { return io_service_; }

    void LogError(const boost::string_ref message) override {
        log_error_(message);
    }

    void LogWarning(const boost::string_ref message) override {
        log_warn_(message);
    }

    void LogNotice(const boost::string_ref message) override {
        log_notice_(message);
    }

    void LogDebug(const boost::string_ref message) override {
        log_debug_(message);
    }


private:
    Request::Properties::ptr_t default_connection_properties_;
    boost::asio::io_service io_service_;
    unique_ptr<boost::asio::io_service::work> work_;
    unique_ptr<ConnectionPool> pool_;

    logger_t log_error_ = [](const boost::string_ref msg) { std::clog << "ERROR: " << msg << std::endl; };
    logger_t log_warn_ = [](const boost::string_ref msg) { std::clog << "WARN: " << msg << std::endl; };
    logger_t log_notice_ = [](const boost::string_ref msg) { std::clog << "NOTICE: " << msg << std::endl; };
    logger_t log_debug_ = [](const boost::string_ref msg) { std::clog << "DEBUG: " << msg << std::endl; };

};

unique_ptr<RestClient> RestClient::Create(boost::optional<Request::Properties> properties) {
    return make_unique<RestClientImpl>(properties);
}

} // restc_cpp
