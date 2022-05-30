

#include <iostream>
#include <thread>
#include <future>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/RequestBody.h"
#include "restc-cpp/internals/helpers.h"

#ifdef RESTC_CPP_WITH_TLS
#   include "boost/asio/ssl.hpp"
#   include "boost/asio/ssl/context.hpp"
#endif

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
            RESTC_CPP_LOG_TRACE_("Done-handler is created");
            ++parent_.current_tasks_;
        }

        ~DoneHandlerImpl() override {
            RESTC_CPP_LOG_TRACE_("Done-handler is destroyed");
            if (--parent_.current_tasks_ == 0) {
                parent_.OnNoMoreWork();
            }
        }

        DoneHandlerImpl(const DoneHandlerImpl&) = delete;
        DoneHandlerImpl(DoneHandlerImpl&&) = delete;

        DoneHandlerImpl& operator = (const DoneHandlerImpl&) = delete;
        DoneHandlerImpl& operator = (DoneHandlerImpl&&) = delete;

    private:
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

        unique_ptr< Reply > Options(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::OPTIONS, rc_);
            return Request(*req);
        }

        unique_ptr< Reply > Head(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::HEAD, rc_);
            return Request(*req);
        }

        unique_ptr< Reply > Patch(string url) override {
            auto req = Request::Create(url, restc_cpp::Request::Type::PATCH, rc_);
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

    RestClientImpl(const boost::optional<Request::Properties>& properties,
                   bool useMainThread)
    : ioservice_instance_{make_unique<boost::asio::io_service>()}
    {
#ifdef RESTC_CPP_WITH_TLS
		setDefaultSSLContext();
#endif
		io_service_ = ioservice_instance_.get();
        Init(properties, useMainThread);
    }

#ifdef RESTC_CPP_WITH_TLS
    RestClientImpl(const boost::optional<Request::Properties>& properties,
        bool useMainThread, shared_ptr<boost::asio::ssl::context> ctx)
        : ioservice_instance_{ make_unique<boost::asio::io_service>() }
    {
        tls_context_ = move(ctx);
        io_service_ = ioservice_instance_.get();
        Init(properties, useMainThread);
    }

    RestClientImpl(const boost::optional<Request::Properties>& properties,
        bool useMainThread, shared_ptr<boost::asio::ssl::context> ctx,
        boost::asio::io_service& ioservice)
        : io_service_{ &ioservice }
    {
        tls_context_ = move(ctx);
        io_service_ = ioservice_instance_.get();
        Init(properties, useMainThread);
    }
#endif

    RestClientImpl(const boost::optional<Request::Properties>& properties,
                   boost::asio::io_service& ioservice)
    : io_service_{&ioservice}
    {
#ifdef RESTC_CPP_WITH_TLS
        setDefaultSSLContext();
#endif
        Init(properties, true);
    }

    ~RestClientImpl() override {
        CloseWhenReady(false);

        for(auto &thread : threads_) {
            if (thread) {
                thread->join();
            }
        }
    }

    RestClientImpl(const RestClientImpl&) = delete;
    RestClientImpl(RestClientImpl&&) = delete;

    RestClientImpl& operator = (const RestClientImpl&) = delete;
    RestClientImpl& operator = (RestClientImpl&&) = delete;

    void Init(const boost::optional<Request::Properties>& properties,
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

        std::vector<promise<void>> wait;

#ifndef  RESTC_CPP_THREADED_CTX
        if (default_connection_properties_->threads != 1) {
            RESTC_CPP_LOG_WARN_("Init: Compiled without RESTC_CPP_THREADED_CTX: Using only one thread!");
            default_connection_properties_->threads = 1;
        }
#endif

        // Make sure we don't re-arrange the vectors while some thread is reading from it
        done_mutexes_.reserve(default_connection_properties_->threads);
        threads_.reserve(default_connection_properties_->threads);

        for(size_t i = 0; i < default_connection_properties_->threads; ++i) {
            wait.emplace_back();
            done_mutexes_.push_back(make_unique<recursive_mutex>());
        }

        work_ = make_unique<boost::asio::io_service::work>(*io_service_);

        RESTC_CPP_LOG_TRACE_("Starting " <<default_connection_properties_->threads << " worker thread(s)");
        for(size_t i = 0; i < default_connection_properties_->threads; ++i) {
            threads_.emplace_back(make_unique<thread>([i, this, &wait]() {
                lock_guard<recursive_mutex> lock(*done_mutexes_.at(i));
                try {
                    wait.at(i).set_value();
                    RESTC_CPP_LOG_DEBUG_("Worker " << i << " is starting.");
                    io_service_->run();
                    RESTC_CPP_LOG_DEBUG_("Worker " << i << " is done.");
                } catch (const exception& ex) {
                    RESTC_CPP_LOG_ERROR_("Worker " << i << " caught exception: " << ex.what());
                }
            }));
        }

        // Wait for the therads to be started
        for(auto& w : wait) {
            w.get_future().get();
        }

        RESTC_CPP_LOG_TRACE_("All worker threads have started");
    }


    Request::Properties::ptr_t GetConnectionProperties() const override {
        return default_connection_properties_;
    }

    bool IsClosed() const noexcept override {
        return closed_;
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
            RESTC_CPP_LOG_TRACE_("CloseWhenReady: Waiting for work to end.");

            // We have to lock/unlock each of them to make sure that all the threads are done
            for(auto& dm : done_mutexes_) {
                if (dm) {
                    lock_guard<recursive_mutex> lock(*dm);
                }
            }
            RESTC_CPP_LOG_TRACE_("CloseWhenReady: Done waiting for work to end.");
        }
    }

    void ClearWork() {
        if (closed_) {
            return;
        }

        if (!io_service_->stopped()) {
            call_once(close_once_, [&] {
                auto promise = make_shared<std::promise<void>>();

                io_service_->dispatch([this, promise]() {
                    LOCK_;
                    if (work_) {
                        work_.reset();
                    }
                    closed_ = true;
                    promise->set_value();
                });

                // Wait for the lambda to finish;
                promise->get_future().get();
            });
        }
    }

    void ProcessInWorker(boost::asio::yield_context yield,
                         const prc_fn_t& fn,
                         const shared_ptr<promise<void>>& promise) {

        ContextImpl ctx(yield, *this);

        DoneHandlerImpl handler(*this);
        try {
            fn(ctx);
        } catch(exception& ex) {
            RESTC_CPP_LOG_ERROR_("ProcessInWorker: Caught exception: " << ex.what());
            if (promise) {
                promise->set_exception(current_exception());
                return;
            }

            throw;
        } catch(...) {
            RESTC_CPP_LOG_ERROR_("*** ProcessInWorker: Caught unknown exception");
            if (promise) {
                promise->set_exception(current_exception());
                return;
            }
            throw;
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

#ifdef RESTC_CPP_WITH_TLS
    shared_ptr<boost::asio::ssl::context> GetTLSContext() override { return tls_context_; }
#endif

    void OnNoMoreWork() {
        RESTC_CPP_LOG_TRACE_("OnNoMoreWork: enter");
        LOCK_;
        if (current_tasks_ > 0) {
            // Cannot close down quite yet.
            RESTC_CPP_LOG_TRACE_("OnNoMoreWork: leaving - we have active tasks");
            return;
        }
        if (closed_ && pool_) {
            call_once(close_pool_once_, [&] {
                RESTC_CPP_LOG_TRACE_("OnNoMoreWork: closing pool");
                pool_->Close();
                pool_.reset();
            });
        }
        if (closed_ && ioservice_instance_) {
            if (!work_ && !io_service_->stopped()) {
                call_once(close_ioservice_once_, [&] {
                    RESTC_CPP_LOG_TRACE_("OnNoMoreWork: Stopping ioservice");
                    io_service_->stop();
                });
            }
        }
        RESTC_CPP_LOG_TRACE_("OnNoMoreWork: leave");
    }

protected:
    unique_ptr<DoneHandler> GetDoneHandler() override {
        return make_unique<DoneHandlerImpl>(*this);
    }

private:
    Request::Properties::ptr_t default_connection_properties_ = make_shared<Request::Properties>();
    unique_ptr<boost::asio::io_service> ioservice_instance_;
    boost::asio::io_service *io_service_ = nullptr;
    ConnectionPool::ptr_t pool_;
    unique_ptr<boost::asio::io_service::work> work_;
#ifdef RESTC_CPP_THREADED_CTX
    atomic_size_t current_tasks_{0};
#else
    size_t current_tasks_{0};
#endif
    bool closed_ = false;
    once_flag close_once_;
    std::vector<unique_ptr<thread>> threads_;
    std::vector<std::unique_ptr<recursive_mutex>> done_mutexes_;
    std::once_flag close_pool_once_;
    std::once_flag close_ioservice_once_;


#ifdef RESTC_CPP_WITH_TLS
    shared_ptr<boost::asio::ssl::context> tls_context_;

    void setDefaultSSLContext() {
        tls_context_ = make_shared<boost::asio::ssl::context>(boost::asio::ssl::context{ boost::asio::ssl::context::sslv23_client });
        tls_context_->set_options(boost::asio::ssl::context::default_workarounds
        | boost::asio::ssl::context::no_sslv2
        | boost::asio::ssl::context::no_sslv3);
    }
#endif

#ifdef RESTC_CPP_THREADED_CTX
    mutable std::mutex mutex_;
#endif
};

unique_ptr<RestClient> RestClient::Create() {
    boost::optional<Request::Properties> properties;
    return make_unique<RestClientImpl>(properties, false);
}

#ifdef RESTC_CPP_WITH_TLS
unique_ptr<RestClient> RestClient::Create(std::shared_ptr<boost::asio::ssl::context> ctx) {
    boost::optional<Request::Properties> properties;
    return make_unique<RestClientImpl>(properties, false, move(ctx));
}

std::unique_ptr<RestClient> RestClient::Create(std::shared_ptr<boost::asio::ssl::context> ctx,
                                               const boost::optional<Request::Properties> &properties)
{
    return make_unique<RestClientImpl>(properties, false, move(ctx));
}

std::unique_ptr<RestClient> RestClient::Create(std::shared_ptr<boost::asio::ssl::context> ctx,
                                               const boost::optional<Request::Properties> &properties,
                                               boost::asio::io_service &ioservice)
{
    return make_unique<RestClientImpl>(properties, false, move(ctx), ioservice);
}

#endif

unique_ptr<RestClient> RestClient::Create(
    const boost::optional<Request::Properties>& properties) {
    return make_unique<RestClientImpl>(properties, false);
}

unique_ptr<RestClient> RestClient::CreateUseOwnThread(const boost::optional<Request::Properties>& properties) {
    return make_unique<RestClientImpl>(properties, true);
}

unique_ptr<RestClient> RestClient::CreateUseOwnThread() {
    return make_unique<RestClientImpl>(boost::optional<Request::Properties>{},
                                       true);
}

std::unique_ptr<RestClient>
RestClient::Create(const boost::optional<Request::Properties>& properties,
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
