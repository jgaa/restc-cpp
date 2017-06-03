

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/RequestBody.h"

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
            RESTC_CPP_LOG_TRACE << "Done-handler is created";
            ++parent_.current_tasks_;
        }

        ~DoneHandlerImpl() {
            RESTC_CPP_LOG_TRACE << "Done-handler is destroyed";
            if (--parent_.current_tasks_ == 0) {
                parent_.OnNoMoreWork();
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

        unique_ptr<Reply> Get(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::GET, rc_);
            return Request(*req);
        }

        unique_ptr< Reply > Post(string url, string body) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::POST, rc_,
                                       {RequestBody::CreateStringBody(move(body))});
            return Request(*req);
        }

        unique_ptr< Reply > Put(string url, string body) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::PUT, rc_,
                                       {RequestBody::CreateStringBody(move(body))});
            return Request(*req);
        }

        unique_ptr< Reply > Delete(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::DELETE, rc_);
            return Request(*req);
        }

        unique_ptr<Reply> Request(restc_cpp::Request& req) override {
            return req.Execute(*this);
        }

        void Sleep(const boost::posix_time::microseconds& ms) override {
            boost::asio::deadline_timer timer(
                GetClient().GetIoService(),
                boost::posix_time::microseconds(ms));

            timer.async_wait(GetYield());
        }

    private:
        boost::asio::yield_context& yield_;
        RestClient& rc_;
    };

    RestClientImpl(boost::optional<Request::Properties> properties,
                   bool useMainThread)
    : ioservice_instance_{make_unique<boost::asio::io_service>()}
    {
        io_service_ = ioservice_instance_.get();
        Init(properties, useMainThread);
    }

    RestClientImpl(boost::optional<Request::Properties> properties,
                   boost::asio::io_service& ioservice)
    : io_service_{&ioservice}
    {
        Init(properties, true);
    }

    ~RestClientImpl() {
        CloseWhenReady(false);
        if (thread_) {
            thread_->join();
        }
    }

    void Init(boost::optional<Request::Properties>& properties,
              bool useMainThread) {

        static const string content_type{"Content-Type"};
        static const string json_type{"application/json; charset=utf-8"};

        if (properties)
            *default_connection_properties_ = *properties;

        if (default_connection_properties_->headers.find(content_type)
            == default_connection_properties_->headers.end()) {
            default_connection_properties_->headers[content_type] = json_type;
        }

        pool_ = ConnectionPool::Create(*this);

        if (useMainThread) {
            return;
        }

        promise<void> wait;
        auto done = wait.get_future();

        thread_ = make_unique<thread>([&]() {
            lock_guard<decltype(done_mutex_)> lock(done_mutex_);
            work_ = make_unique<boost::asio::io_service::work>(*io_service_);
            wait.set_value();
            RESTC_CPP_LOG_DEBUG << "Worker is starting.";
            io_service_->run();
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
        if (!io_service_->stopped()) {
            io_service_->dispatch([this](){
                if (current_tasks_ == 0) {
                    OnNoMoreWork();
                }
            });
        }
        if (wait) {
            RESTC_CPP_LOG_TRACE << "CloseWhenReady: Waiting for work to end.";
            lock_guard<decltype(done_mutex_)> lock(done_mutex_);
            RESTC_CPP_LOG_TRACE << "CloseWhenReady: Done waiting for work to end.";
        }
    }

    void ClearWork() {
        if (closed_) {
            return;
        }

        if (!io_service_->stopped()) {
            auto promise = make_shared<std::promise<void>>();

            io_service_->dispatch([this, promise]() {
                if (work_) {
                    work_.reset();
                }
                closed_ = true;
                promise->set_value();
            });

            // Wait for the lambda to finish;
            promise->get_future().get();
        }
    }

    const void
    ProcessInWorker(boost::asio::yield_context yield,
                    const prc_fn_t& fn,
                    const shared_ptr<promise<void>>& promise) {

        ContextImpl ctx(yield, *this);

        DoneHandlerImpl handler(*this);
        try {
            fn(ctx);
        } catch(exception& ex) {
            RESTC_CPP_LOG_ERROR << "Caught exception: " << ex.what();
            if (promise) {
                promise->set_exception(current_exception());
            }
            return;
        } catch(...) {
            RESTC_CPP_LOG_ERROR << "*** Caught unknown exception";
            if (promise) {
                promise->set_exception(current_exception());
            }
            return;
        }


        if (promise) {
            promise->set_value();
        }
    }

    void Process(const prc_fn_t& fn) override {
        boost::asio::spawn(*io_service_,
                           bind(&RestClientImpl::ProcessInWorker, this,
                                placeholders::_1, fn, nullptr));
    }

    future< void > ProcessWithPromise(const prc_fn_t& fn) override {
        auto promise = make_shared<std::promise<void>>();
        auto future = promise->get_future();

        boost::asio::spawn(*io_service_,
                           bind(&RestClientImpl::ProcessInWorker, this,
                                placeholders::_1, fn, promise));

        return future;
    }

    std::shared_ptr<ConnectionPool> GetConnectionPool() override {
        assert(pool_);
        return pool_;
    }

    boost::asio::io_service& GetIoService() override { return *io_service_; }

    void OnNoMoreWork() {
        if (closed_ && pool_) {
            pool_->Close();
            pool_.reset();
        }
        if (closed_ && ioservice_instance_) {
            if (!work_ && !io_service_->stopped()) {
                io_service_->stop();
            }
        }
    }

protected:
    unique_ptr<DoneHandler> GetDoneHandler() override {
        return make_unique<DoneHandlerImpl>(*this);
    }

private:
    Request::Properties::ptr_t default_connection_properties_ = make_shared<Request::Properties>();
    boost::asio::io_service *io_service_ = nullptr;
    ConnectionPool::ptr_t pool_;
    unique_ptr<boost::asio::io_service::work> work_;
    size_t current_tasks_ = 0;
    bool closed_ = false;
    unique_ptr<thread> thread_;
    recursive_mutex done_mutex_;
    unique_ptr<boost::asio::io_service> ioservice_instance_;

};

unique_ptr<RestClient> RestClient::Create() {
    boost::optional<Request::Properties> properties;
    return make_unique<RestClientImpl>(properties, false);
}


unique_ptr<RestClient> RestClient::Create(
    boost::optional<Request::Properties> properties) {
    return make_unique<RestClientImpl>(properties, false);
}

unique_ptr<RestClient> RestClient::CreateUseOwnThread(boost::optional<Request::Properties> properties) {
    return make_unique<RestClientImpl>(properties, true);
}

unique_ptr<RestClient> RestClient::CreateUseOwnThread() {
    return make_unique<RestClientImpl>(boost::optional<Request::Properties>{},
                                       true);
}

std::unique_ptr<RestClient>
RestClient::Create(boost::optional<Request::Properties> properties,
       boost::asio::io_service& ioservice) {
    return make_unique<RestClientImpl>(properties, ioservice);
}

std::unique_ptr<RestClient>
RestClient::Create(boost::asio::io_service& ioservice) {
    return make_unique<RestClientImpl>(boost::optional<Request::Properties>{},
                                       ioservice);
}

unique_ptr<Context> Context::Create(boost::asio::yield_context& yield,
                                                RestClient& rc) {
    return make_unique<RestClientImpl::ContextImpl>(yield, rc);
}

} // restc_cpp
