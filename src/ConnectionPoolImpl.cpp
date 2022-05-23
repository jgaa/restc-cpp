#include <atomic>
#include <iostream>
#include <thread>
#include <future>
#include <queue>
#include <mutex>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/error.h"
#include "restc-cpp/internals/helpers.h"

#include "ConnectionImpl.h"
#include "SocketImpl.h"

#ifdef RESTC_CPP_WITH_TLS
#   include "TlsSocketImpl.h"
#endif

using namespace std;

namespace restc_cpp {

class ConnectionPoolImpl
    : public ConnectionPool
    , public std::enable_shared_from_this<ConnectionPoolImpl> {
public:

    struct Key {
        Key(boost::asio::ip::tcp::endpoint ep,
            const Connection::Type connectionType)
        : endpoint{move(ep)}, type{connectionType} {}

        Key(const Key&) = default;
        Key(Key&&) = default;
        ~Key() = default;
        Key& operator = (const Key&) = delete;
        Key& operator = (Key&&) = delete;

        bool operator < (const Key& key) const {
            if (static_cast<int>(type) < static_cast<int>(key.type)) {
                return true;
            }

            return endpoint < key.endpoint;
        }

        friend std::ostream& operator << (std::ostream& o, const Key& v) {
            return o << "{Key "
                << (v.type == Connection::Type::HTTPS? "https" : "http")
                << "://"
                << v.endpoint
                << "}";
        }

    private:
        const boost::asio::ip::tcp::endpoint endpoint;
        const Connection::Type type;
    };

    struct Entry {
        using timestamp_t = decltype(chrono::steady_clock::now());
        using ptr_t = std::shared_ptr<Entry>;

        Entry(boost::asio::ip::tcp::endpoint ep,
              const Connection::Type connectionType,
              Connection::ptr_t conn,
              const Request::Properties& prop)
        : key{move(ep), connectionType}, connection{move(conn)}, ttl{prop.cacheTtlSeconds}
        , created{time(nullptr)} {}

        friend ostream& operator << (ostream& o, const Entry& e) {
            o << "{Entry " << e.key;
            if (e.connection) {
                o << ' ' << *e.connection;
            } else {
                o << "No connection";
            }
            return o << '}';
        }

        const Key& GetKey() const noexcept { return key; }
        Connection::ptr_t& GetConnection() noexcept { return connection; }
        int GetTtl() const noexcept { return ttl; }
        time_t GetCreated() const noexcept { return created;}
        timestamp_t GetLastUsed() const noexcept {
            LOCK_ALWAYS_;
            return last_used;
        }
        void SetLastUsed(timestamp_t ts) {
            LOCK_ALWAYS_;
            last_used = ts;
        }

    private:
        const Key key;
        Connection::ptr_t connection;
        const int ttl = 60;
        const time_t created;
        mutable std::mutex mutex_;
        timestamp_t last_used = chrono::steady_clock::now();
    };

    // Owns the connection
    class ConnectionWrapper : public Connection
    {
    public:
        using release_callback_t = std::function<void (const Entry::ptr_t&)>;
        ConnectionWrapper(Entry::ptr_t entry,
                        release_callback_t on_release)
        : on_release_{move(on_release)}, entry_{move(entry)}
        {
        }

        ConnectionWrapper() = delete;
        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper(ConnectionWrapper&&) = delete;
        ConnectionWrapper& operator = (const ConnectionWrapper& ) = delete;
        ConnectionWrapper& operator = (ConnectionWrapper&& ) = delete;

        Socket& GetSocket() override {
            return entry_->GetConnection()->GetSocket();
        }

        const Socket& GetSocket() const override {
             return entry_->GetConnection()->GetSocket();
        }

        boost::uuids::uuid GetId() const override {
            return entry_->GetConnection()->GetId();
        }

        ~ConnectionWrapper() override {
            if (on_release_) {
                on_release_(entry_);
            }
        }

    private:
        release_callback_t on_release_;
        shared_ptr<Entry> entry_;
    };


    ConnectionPoolImpl(RestClient& owner)
    : owner_{owner}, properties_{owner.GetConnectionProperties()}
    , cache_cleanup_timer_{owner.GetIoService()}
#ifdef RESTC_CPP_THREADED_CTX
    , strand_{owner_.GetIoService()}
#endif

    {
        on_release_ = [this](const Entry::ptr_t& entry) { OnRelease(entry); };
    }

    Connection::ptr_t
    GetConnection(const boost::asio::ip::tcp::endpoint ep,
                const Connection::Type connectionType,
                bool newConnectionPlease) override {

        if (!newConnectionPlease) {
            if (auto conn = GetFromCache(ep, connectionType)) {
                RESTC_CPP_LOG_TRACE_("Reusing connection from cache "
                    << *conn);
                return conn;
            }

            if (!CanCreateNewConnection(ep, connectionType)) {
                throw ConstraintException(
                    "Cannot create connection - too many connections");
            }
        }

        return CreateNew(ep, connectionType);
    }

    // Get ctx for internal, syncronized operations;
#ifdef RESTC_CPP_THREADED_CTX
    auto GetCtx() const {
      return strand_;
    }
#else
    auto& GetCtx() const {
      return owner_.GetIoService();
    }
#endif

    size_t GetIdleConnections() const override {
        LOCK_ALWAYS_;
        return idle_.size();
    }

    void Close() override {
        if (!closed_) {
            call_once(close_once_, [this] {
                closed_ = true;
                LOCK_ALWAYS_;
                cache_cleanup_timer_.cancel();
                idle_.clear();
            });
        }
    }

    void StartTimer() {
        ScheduleNextCacheCleanup();
    }

private:
    void ScheduleNextCacheCleanup() {
        LOCK_ALWAYS_;
        cache_cleanup_timer_.expires_from_now(
            boost::posix_time::seconds(properties_->cacheCleanupIntervalSeconds));
        cache_cleanup_timer_.async_wait(std::bind(&ConnectionPoolImpl::OnCacheCleanup,
                                                  shared_from_this(), std::placeholders::_1));
    }

    void OnCacheCleanup(const boost::system::error_code& error) {
        if (closed_) {
            return;
        }

        if (error) {
            RESTC_CPP_LOG_DEBUG_("OnCacheCleanup: " << error);
            return;
        }

        RESTC_CPP_LOG_TRACE_("Cleaning cache...");

        const auto now = std::chrono::steady_clock::now();
        {
            LOCK_ALWAYS_;
            for(auto it = idle_.begin(); !closed_ && it != idle_.end();) {

                auto current = it;
                ++it;

                const auto& entry = *current->second;
                auto expires = entry.GetLastUsed() + std::chrono::seconds(entry.GetTtl());
                if (expires < now) {
                    RESTC_CPP_LOG_TRACE_("Expiring " << *current->second->GetConnection());
                    idle_.erase(current);
                } else {
                    RESTC_CPP_LOG_TRACE_("Keeping << " << *current->second->GetConnection()
                        << " expieres in "
                        << std::chrono::duration_cast<std::chrono::seconds>(expires - now).count()
                        << " seconds ");
                }
            }
        }

        ScheduleNextCacheCleanup();
    }

    void OnRelease(const Entry::ptr_t entry) {
        {
            LOCK_ALWAYS_;
            in_use_.erase(entry->GetKey());
        }
        if (closed_ || !entry->GetConnection()->GetSocket().IsOpen()) {
            RESTC_CPP_LOG_TRACE_("Discarding " << *entry << " after use");
            return;
        }

        RESTC_CPP_LOG_TRACE_("Recycling " << *entry << " after use");
        entry->SetLastUsed(chrono::steady_clock::now());
        {
            LOCK_ALWAYS_;
            idle_.insert({entry->GetKey(), entry});
        }
    }

    // Check the constraints to see if we can create a new connection
    bool CanCreateNewConnection(const boost::asio::ip::tcp::endpoint& ep,
                                const Connection::Type connectionType) {
        if (closed_) {
            throw ObjectExpiredException("The connection-pool is closed.");
        }

        promise<bool> pr;
        auto result = pr.get_future();

        size_t cnt = 0;
        const auto key = Key{ep, connectionType};
        {
            LOCK_ALWAYS_;
            cnt = idle_.count(key) + in_use_.count(key);
        }

        if (cnt >= properties_->cacheMaxConnectionsPerEndpoint) {
            RESTC_CPP_LOG_DEBUG_("No more available slots for " << key);
            pr.set_value(false);
            return false;
        }


        {
            LOCK_ALWAYS_;
            cnt = idle_.size() + in_use_.size();
        }
        if (cnt >= properties_->cacheMaxConnections) {

            // See if we can release an idle connection.
            if (!PurgeOldestIdleEntry()) {
                RESTC_CPP_LOG_DEBUG_("No more available slots (max="
                    << properties_->cacheMaxConnections
                    << ", used=" << all_cnt << ')');
                    pr.set_value(false);
                    return false;
            }
        }

        return true;
    }

    bool PurgeOldestIdleEntry() {
        LOCK_ALWAYS_;
        auto oldest =  idle_.begin();
        for (auto it = idle_.begin(); it != idle_.end(); ++it) {
            if (it->second->GetLastUsed() < oldest->second->GetLastUsed()) {
                oldest = it;
            }
        }

        if (oldest != idle_.end()) {
            RESTC_CPP_LOG_TRACE_("LRU-Purging " << *oldest->second);
            idle_.erase(oldest);
            return true;
        }

        return false;
    }

    // Get a connection from the cache if it's there.
    Connection::ptr_t GetFromCache(const boost::asio::ip::tcp::endpoint& ep,
                                   const Connection::Type connectionType) {
        if (closed_) {
            throw ObjectExpiredException("The connection-pool is closed.");
        }

        promise<Connection::ptr_t> pr;
        auto result = pr.get_future();

        LOCK_ALWAYS_;
        const auto key = Key{ep, connectionType};
        auto it = idle_.find(key);
        if (it != idle_.end()) {
            auto wrapper = make_unique<ConnectionWrapper>(it->second, on_release_);
            in_use_.insert(*it);
            idle_.erase(it);
            return wrapper;
        }

        return {};
    }

    Connection::ptr_t CreateNew(const boost::asio::ip::tcp::endpoint& ep,
                                const Connection::Type connectionType) {
        unique_ptr<Socket> socket;
        if (connectionType == Connection::Type::HTTP) {
            socket = make_unique<SocketImpl>(owner_.GetIoService());
        }
        else {
#ifdef RESTC_CPP_WITH_TLS
            socket = make_unique<TlsSocketImpl>(owner_.GetIoService(), owner_.GetTLSContext());
#else
            throw NotImplementedException(
                "restc_cpp is compiled without TLS support");
#endif
        }

        auto entry = make_shared<Entry>(ep, connectionType,
                                        make_shared<ConnectionImpl>(move(socket)),
                                        *properties_);

        RESTC_CPP_LOG_TRACE_("Created new connection " << *entry);

        promise<Connection::ptr_t> pr;
        auto result = pr.get_future();

        {
            LOCK_ALWAYS_;
            in_use_.insert({entry->GetKey(), entry});
        }
        return make_unique<ConnectionWrapper>(entry, on_release_);
    }

#ifdef RESTC_CPP_THREADED_CTX
    std::atomic_bool closed_{false};
#else
    bool closed_ = false;
#endif
    std::once_flag close_once_;
    RestClient& owner_;
    multimap<Key, Entry::ptr_t> idle_;
    multimap<Key, std::weak_ptr<Entry>> in_use_;
    std::queue<Entry> pending_;
    const Request::Properties::ptr_t properties_;
    ConnectionWrapper::release_callback_t on_release_;
    boost::asio::deadline_timer cache_cleanup_timer_;

    mutable std::mutex mutex_;
    boost::asio::io_context::strand strand_;
}; // ConnectionPoolImpl


ConnectionPool::ptr_t
ConnectionPool::Create(RestClient& owner) {
    auto instance = make_shared<ConnectionPoolImpl>(owner);
    instance->StartTimer();
    return instance;
}


} // restc_cpp

