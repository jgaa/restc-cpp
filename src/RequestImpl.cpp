#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"

using namespace std;

namespace restc_cpp {

class RequestImpl : public Request {
public:

    RequestImpl(const std::string& url,
                const Type requestType,
                RestClient& owner,
                const args_t *args,
                const headers_t *headers)
    : url_{url}, type_{requestType}, owner_{owner}
    {
        if (args || headers) {
            properties_ = make_shared<Properties>(*owner_.GetConnectionProperties());
            merge_map(args, properties_->args);
            merge_map(headers, properties_->headers);
        } else {
            properties_ = owner_.GetConnectionProperties();
        }
    }

    const Properties& GetProperties() const override {
        return *properties_;
    }

    void SetProperties(Properties::ptr_t propreties) override {
        properties_ = move(propreties);
    }

    const std::string& Verb(const Type requestType) {
        static const std::vector<std::string> names =
            { "GET", "POST", "PUT", "DELETE" };

        return names[static_cast<int>(requestType)];
    }

    unique_ptr<Reply> Execute(Context& ctx) override {
        static const std::string crlf{"\r\n"};
        static const std::string column{": "};

        std::ostringstream request_buffer;
        Connection::Type protocol_type = Connection::Type::HTTP;

        // Extract and resolve the protocol, hostname and port
        boost::string_ref protocol(url_);
        if (protocol.find("https://") == 0) {
            protocol = boost::string_ref(url_.c_str(), 8);
            protocol_type = Connection::Type::HTTPS;
        } else if (protocol.find("http://") == 0) {
            protocol = boost::string_ref(url_.c_str(), 7);
        } else {
            throw invalid_argument("Invalid protocol in url. Must be 'http[s]://'");
        }

        boost::string_ref host(protocol.end());
        boost::string_ref port("80");
        boost::string_ref path("/");
        if (auto pos = host.find(':') != host.npos) {
            // TODO: Add a regex to verify the URL and extract data.
            if (host.length() <= static_cast<decltype(host.length())>(pos + 2)) {
                throw invalid_argument("Invalid host (no port after column)");
            }
            port = boost::string_ref(&host[pos+1]);
            host = boost::string_ref(host.data(), pos);
            if (auto path_start = port.find('/') != port.npos) {
                path = &port[path_start];
                port = boost::string_ref(port.data(), path_start);
            }
        } else {
            if (auto path_start = host.find('/') != host.npos) {
                path = &host[path_start];
                host = boost::string_ref(host.data(), path_start);
            }
        }

        // Log
        {
            std::ostringstream msg;
            msg << "Host: '" << host << "', port: '" << port << "'.";
            owner_.LogDebug(msg);
        }

        // TODO: Add args
        std::string args;

        // Build the request-path
        request_buffer << Verb(type_) << ' ' << path << args << "HTTP/1.1" << crlf;

        // Build the header buffers
        const headers_t& headers = properties_->headers;
        if (headers.find("Host") == headers.end()) {
            request_buffer << "Host: " << host << crlf;
        }

        if (headers.find("Content-Lenght") == headers.end()) {
            request_buffer << "Content-Lenght: " << body_.size() << crlf;
        }

        for(const auto& it : headers) {
            request_buffer << it.first << column << it.second << crlf;
        }

        // Prepare the body
        request_buffer << crlf << body_;

        // TODO: Remove logging here. Too expensive. We can log from async write.
        owner_.LogDebug(request_buffer.str().c_str());

        // Prepare a reply object
        auto reply = Reply::Create(ctx, *this, owner_);
        const int max_retries = 3;

        /* Loop three times (on IO error with no data received) over each IP
         * resolved from the hostname. Third time, ask for a new connection.
         */

        for(int attempts = 1; attempts <= max_retries; ++attempts) {
            // Resolve the hostname
            boost::asio::ip::tcp::resolver resolver(owner_.GetIoService());
            auto address_it = resolver.async_resolve({host.data(), port.data()},
                                                     ctx.GetYield());

            decltype(address_it) addr_end;
            for(; address_it != addr_end; ++address_it) {

                const auto endpoint = address_it->endpoint();

                // Get a connection from the pool
                auto connection = owner_.GetConnectionPool().GetConnection(
                    endpoint, protocol_type, attempts == max_retries);

                // Connect if the connection is new.
                if (!connection->GetSocket().GetSocket().is_open()) {
                    // TODO: Set connect timeout
                    connection->GetSocket().AsyncConnect(endpoint, ctx.GetYield());
                }

                // Send the request.
                connection->GetSocket().AsyncWrite(ToBuffer(request_buffer), ctx.GetYield());

                // Pass IO resposibility to the Reply

                /* Return the reply. At this time the reply headers and body is returned,
                 * and the connection can be returned to the connection-pool.
                 */

                return reply;
            }
        }

        throw runtime_error("Failed to connect");
    }

private:
    const std::string url_;
    const Type type_;
    std::string body_;
    Properties::ptr_t properties_;
    RestClient &owner_;
};


std::unique_ptr<Request>
Request::Create(const std::string& url,
       const Type requestType,
       RestClient& owner,
       const args_t *args,
       const headers_t *headers) {

    return make_unique<RequestImpl>(url, requestType, owner, args, headers);
}

} // restc_cpp

