#include <iostream>
#include <thread>
#include <future>
#include <queue>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/error.h"

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
        timestamp_t GetLastUsed() const noexcept { return last_used; }

    private:
        const Key key;
        Connection::ptr_t connection;
        const int ttl = 60;
        const time_t created;
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

    {
        on_release_ = [this](const Entry::ptr_t& entry) { OnRelease(entry); };
    }

    Connection::ptr_t
    GetConnection(const boost::asio::ip::tcp::endpoint ep,
                const Connection::Type connectionType,
                bool newConnectionPlease) override {

        if (!newConnectionPlease) {
            if (auto conn = GetFromCache(ep, connectionType)) {
                RESTC_CPP_LOG_TRACE
                    << "Reusing connection from cache "
                    << *conn;
                return conn;
            }

            if (!CanCreateNewConnection(ep, connectionType)) {
                throw ConstraintException(
                    "Cannot create connection - too many connections");
            }
        }

        return CreateNew(ep, connectionType);
    }

    std::future<std::size_t> GetIdleConnections() const override {
        auto my_promise = make_shared<promise<size_t>>() ;
        owner_.GetIoService().dispatch([my_promise, this]() {
            my_promise->set_value(idle_.size());
        });
        return my_promise->get_future();
    }

    void Close() override {
        if (!closed_) {
            closed_ = true;
            cache_cleanup_timer_.cancel();
            idle_.clear();
        }
    }

    void StartTimer() {
        ScheduleNextCacheCleanup();
    }

private:
    void ScheduleNextCacheCleanup() {
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
            RESTC_CPP_LOG_DEBUG << "OnCacheCleanup: " << error;
            return;
        }

        RESTC_CPP_LOG_TRACE << "Cleaning cache...";

        const auto now = std::chrono::steady_clock::now();
        for(auto it = idle_.begin(); it != idle_.end();) {

            auto current = it;
            ++it;

            const auto& entry = *current->second;
            auto expires = entry.GetLastUsed() + std::chrono::seconds(entry.GetTtl());
            if (expires < now) {
                RESTC_CPP_LOG_TRACE << "Expiring " << *current->second->GetConnection();
                idle_.erase(current);
            } else {
                RESTC_CPP_LOG_TRACE << "Keeping << " << *current->second->GetConnection()
                    << " expieres in "
                    << std::chrono::duration_cast<std::chrono::seconds>(expires - now).count()
                    << " seconds ";
            }
        }

        ScheduleNextCacheCleanup();
    }

    void OnRelease(const Entry::ptr_t& entry) {
        in_use_.erase(entry->GetKey());
        if (closed_ || !entry->GetConnection()->GetSocket().IsOpen()) {
            RESTC_CPP_LOG_TRACE << "Discarding " << *entry << " after use";
            return;
        }

        RESTC_CPP_LOG_TRACE << "Recycling " << *entry << " after use";
        entry->GetLastUsed() = chrono::steady_clock::now();
        idle_.insert({entry->GetKey(), entry});
    }

    // Check the constraints to see if we can create a new connection
    bool CanCreateNewConnection(const boost::asio::ip::tcp::endpoint& ep,
                                const Connection::Type connectionType) {
        if (closed_) {
            throw ObjectExpiredException("The connection-pool is closed.");
        }

        {
            const auto key = Key{ep, connectionType};
            const size_t ep_cnt = idle_.count(key) + in_use_.count(key);
            if (ep_cnt >= properties_->cacheMaxConnectionsPerEndpoint) {
                RESTC_CPP_LOG_DEBUG
                    << "No more available slots for " << key;
                return false;
            }
        }

        {
            const size_t all_cnt = idle_.size() + in_use_.size();
            if (all_cnt >= properties_->cacheMaxConnections) {

                // See if we can release an idle connection.
                if (!PurgeOldestIdleEntry()) {
                    RESTC_CPP_LOG_DEBUG
                        << "No more available slots (max="
                        << properties_->cacheMaxConnections
                        << ", used=" << all_cnt << ")";
                        return false;
                }
            }
        }

        return true;
    }

    bool PurgeOldestIdleEntry() {
        auto oldest =  idle_.begin();
        for (auto it = idle_.begin(); it != idle_.end(); ++it) {
            if (it->second->GetLastUsed() < oldest->second->GetLastUsed()) {
                oldest = it;
            }
        }

        if (oldest != idle_.end()) {
            RESTC_CPP_LOG_TRACE << "LRU-Purging " << *oldest->second;
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
        const auto key = Key{ep, connectionType};
        auto it = idle_.find(key);
        if (it != idle_.end()) {
            auto wrapper = make_unique<ConnectionWrapper>(it->second, on_release_);
			in_use_.insert(*it);
            idle_.erase(it);
            return move(wrapper);
        }

        return nullptr;
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

        RESTC_CPP_LOG_TRACE << "Created new connection " << *entry;
        in_use_.insert({entry->GetKey(), entry});
        return make_unique<ConnectionWrapper>(entry, on_release_);
    }

    bool closed_ = false;
    RestClient& owner_;
    multimap<Key, Entry::ptr_t> idle_;
    multimap<Key, std::weak_ptr<Entry>> in_use_;
    std::queue<Entry> pending_;
    const Request::Properties::ptr_t properties_;
    ConnectionWrapper::release_callback_t on_release_;
    boost::asio::deadline_timer cache_cleanup_timer_;
}; // ConnectionPoolImpl


ConnectionPool::ptr_t
ConnectionPool::Create(RestClient& owner) {
    auto instance = make_shared<ConnectionPoolImpl>(owner);
    instance->StartTimer();
    return instance;
}


} // restc_cpp

