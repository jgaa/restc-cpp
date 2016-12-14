

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

    /*! Proper shutdown handling
     * We remove the waiter in CloseWhenReady(), but if we have
     * a connection pool, the timer there will still keep the
     * ioservice busy. So we have to manually shut down the
     * ioservice if the waiter has been removed, and we have
     * no more coroutines in flight.
     */
    struct DoneHandlerImpl : public DoneHandler {

        DoneHandlerImpl(RestClientImpl& parent)
        : parent_{parent} {
            ++parent_.current_tasks_;
        }

        ~DoneHandlerImpl() {
            if (--parent_.current_tasks_ == 0) {
                std::lock_guard<decltype(parent_.work_mutex_)> lock(parent_.work_mutex_);
                if (!parent_.work_ && !parent_.io_service_.stopped()) {
                    parent_.io_service_.stop();
                }
            }
        }

        RestClientImpl& parent_;
    };

    class ContextImpl : public Context {
    public:
        ContextImpl(boost::asio::yield_context& yield,
                    RestClient& rc)
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
        RestClient& rc_;
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

        thread_ = make_unique<thread>([&]() {
            std::lock_guard<decltype(done_mutex_)> lock(done_mutex_);
            pool_ = ConnectionPool::Create(*this);
            work_ = make_unique<boost::asio::io_service::work>(io_service_);
            wait.set_value();
            io_service_.run();
            RESTC_CPP_LOG_DEBUG << "Worker is done.";
        });

        // Wait for the ConnectionPool to be constructed
        done.get();
    }

    const Request::Properties::ptr_t GetConnectionProperties() const override {
        return default_connection_properties_;
    }

    void CloseWhenReady(bool wait) override {
        ClearWork();
        if (wait) {
            std::lock_guard<decltype(done_mutex_)> lock(done_mutex_);
        }
    }

    void ClearWork() {
        std::lock_guard<decltype(work_mutex_)> lock(work_mutex_);
        if (work_) {
            work_.reset();
        }
    }

    ~RestClientImpl() {
        ClearWork();
        if (!io_service_.stopped()) {
            io_service_.stop();
        }
        if (thread_) {
            thread_->join();
        }
    }


    const void
    ProcessInWorker(boost::asio::yield_context yield,
                    const prc_fn_t& fn,
                    const std::shared_ptr<std::promise<void>>& promise) {

        ContextImpl ctx(yield, *this);

        DoneHandlerImpl handler(*this);
        try {
            fn(ctx);
        } catch(std::exception& ex) {
            RESTC_CPP_LOG_ERROR << "Caught exception: " << ex.what();
            if (promise) {
                promise->set_exception(std::current_exception());
            }
            return;
        } catch(...) {
            RESTC_CPP_LOG_ERROR << "*** Caught unknown exception";
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

protected:
    std::unique_ptr<DoneHandler> GetDoneHandler() override {
        return make_unique<DoneHandlerImpl>(*this);
    }

private:
    Request::Properties::ptr_t default_connection_properties_;
    boost::asio::io_service io_service_;
    unique_ptr<ConnectionPool> pool_;
    unique_ptr<boost::asio::io_service::work> work_;
    size_t current_tasks_ = 0;
    unique_ptr<std::thread> thread_;
    std::recursive_mutex done_mutex_;
    std::mutex work_mutex_;
};

unique_ptr<RestClient> RestClient::Create(boost::optional<Request::Properties> properties) {
    return make_unique<RestClientImpl>(properties);
}

std::unique_ptr<Context> Context::Create(boost::asio::yield_context& yield,
                                                RestClient& rc) {
    return make_unique<RestClientImpl::ContextImpl>(yield, rc);
}

} // restc_cpp
