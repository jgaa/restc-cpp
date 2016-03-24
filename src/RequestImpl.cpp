#include <iostream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"

// TODO: If we have a ready body in a buffer, send the header and body as two buffers
// TODO: Implement HTTPS transport

using namespace std;

namespace restc_cpp {

class RequestImpl : public Request {
public:

    RequestImpl(const std::string& url,
                const Type requestType,
                RestClient& owner,
                boost::optional<Body> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers)
    : url_{url}, parsed_url_{url.c_str()} , request_type_{requestType}
    , owner_{owner}
    {
        if (args || headers) {
            properties_ = make_shared<Properties>(*owner_.GetConnectionProperties());
            merge_map(args, properties_->args);
            merge_map(headers, properties_->headers);
        } else {
            properties_ = owner_.GetConnectionProperties();
        }

        if (body) {
            if (body->type_ == Body::Type::STRING) {
                assert(body->body_str_);
                body_ = move(*body->body_str_);
            }
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

    void BuildOutgoingRequest(std::ostream& request_buffer) {
        static const std::string crlf{"\r\n"};
        static const std::string column{": "};

        // Build the request-path
        request_buffer << Verb(request_type_) << ' '
            << parsed_url_.GetPath().to_string();

        // Add arguments to the path as ?name=value&name=value...
        bool first_arg = true;
        for(const auto& it : properties_->args) {
            // TODO: Add escaping of strings
            if (first_arg) {
                first_arg = false;
                if (!parsed_url_.GetPath().ends_with('/')) {
                    request_buffer << '/';
                }
                request_buffer << '?';
            } else {
                request_buffer << '&';
            }

            request_buffer << it.first << '=' << it.second;
        }

        request_buffer << " HTTP/1.1" << crlf;

        // Build the header buffers
        const headers_t& headers = properties_->headers;
        if (headers.find("Host") == headers.end()) {
            request_buffer << "Host: " << parsed_url_.GetHost().to_string()
                << crlf;
        }

        if (headers.find("Content-Lenght") == headers.end()) {
            request_buffer << "Content-Lenght: " << body_.size() << crlf;
        }

        for(const auto& it : headers) {
            request_buffer << it.first << column << it.second << crlf;
        }

        // Prepare the body
        request_buffer << crlf << body_;
    }

    unique_ptr<Reply> Execute(Context& ctx) override {
        const Connection::Type protocol_type =
            (parsed_url_.GetProtocol() == Url::Protocol::HTTPS)
            ? Connection::Type::HTTPS
            : Connection::Type::HTTP;

        std::ostringstream request_buffer;
        BuildOutgoingRequest(request_buffer);

        // TODO: Remove logging here. Too expensive. We can log from async write.
        owner_.LogDebug(request_buffer.str().c_str());


        // Prepare a reply object
        const int max_retries = 3;

        /* Loop three times (on connect or write error) over each IP
         * resolved from the hostname. Third time, ask for a new connection.
         */

        boost::asio::ip::tcp::resolver resolver(owner_.GetIoService());
        for(int attempts = 1; attempts <= max_retries; ++attempts) {
            // Resolve the hostname
            const boost::asio::ip::tcp::resolver::query query{
                parsed_url_.GetHost().to_string(),
                parsed_url_.GetPort().to_string()};

            {
                std::ostringstream msg;
                msg << "Resolving " << query.host_name() << ":"
                    << query.service_name() << endl;
                owner_.LogDebug(msg);
            }

            auto address_it = resolver.async_resolve(query,ctx.GetYield());
            decltype(address_it) addr_end;

            for(; address_it != addr_end; ++address_it) {

                const auto endpoint = address_it->endpoint();

                // Get a connection from the pool
                auto connection = owner_.GetConnectionPool().GetConnection(
                    endpoint, protocol_type, attempts == max_retries);

                // Connect if the connection is new.
                if (!connection->GetSocket().GetSocket().is_open()) {
                    // TODO: Set connect timeout
                    std::ostringstream msg;
                    msg << "Connecting to " << endpoint << endl;
                    owner_.LogDebug(msg);

                    try {
                        connection->GetSocket().AsyncConnect(
                            endpoint, ctx.GetYield());
                    } catch(const exception& ex) {
                        std::ostringstream msg;
                        msg << "Connect failed with exception type: "
                            << typeid(ex).name()
                            << ", message: " << ex.what();
                        owner_.LogDebug(msg);
                        continue;
                    }
                }

                // Send the request.
                try {
                    connection->GetSocket().AsyncWrite(
                        ToBuffer(request_buffer), ctx.GetYield());
                } catch(const exception& ex) {
                    std::ostringstream msg;
                    msg << "Write failed with exception type: "
                        << typeid(ex).name()
                        << ", message: " << ex.what();
                    owner_.LogDebug(msg);
                    continue;
                }

                // Pass IO resposibility to the Reply
                auto reply = Reply::Create(connection, ctx, owner_);

                // TODO: Handle redirects
                reply->StartReceiveFromServer();

                /* Return the reply. At this time the reply headers and body
                 * is returned, and the connection can be returned to the
                 * connection-pool.
                 */

                return reply;
            }
        }

        throw runtime_error("Failed to connect");
    }

private:
    const std::string url_;
    const Url parsed_url_;
    const Type request_type_;
    std::string body_;
    Properties::ptr_t properties_;
    RestClient &owner_;
};


std::unique_ptr<Request>
Request::Create(const std::string& url,
                const Type requestType,
                RestClient& owner,
                boost::optional<Body> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers) {

    return make_unique<RequestImpl>(url, requestType, owner, body, args, headers);
}

} // restc_cpp

