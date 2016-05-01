#include <iostream>
#include <fstream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Url.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/IoTimer.h"

using namespace std;

namespace restc_cpp {

class RequestImpl : public Request {
public:

    RequestImpl(const std::string& url,
                const Type requestType,
                RestClient& owner,
                std::unique_ptr<Body> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers)
    : url_{url}, parsed_url_{url_.c_str()} , request_type_{requestType}
    , body_{std::move(body)}, owner_{owner}
    {
        if (args || headers) {
            properties_ = make_shared<Properties>(*owner_.GetConnectionProperties());

            if (args) {
                properties_->args.insert(properties_->args.end(),
                                         args->begin(), args->end());
            }

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

    const uint64_t GetContentBytesSent() const noexcept {
        return bytes_sent_ - header_size_;
    }

private:
    std::string BuildOutgoingRequest() {
        static const std::string crlf{"\r\n"};
        static const std::string column{": "};

        std::ostringstream request_buffer;

        // Build the request-path
        request_buffer << Verb(request_type_) << ' '
            << parsed_url_.GetPath().to_string();

        // Add arguments to the path as ?name=value&name=value...
        bool first_arg = true;
        for(const auto& arg : properties_->args) {
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

            request_buffer << arg.name<< '=' << arg.value;
        }

        request_buffer << " HTTP/1.1" << crlf;

        // Build the header buffers
        const headers_t& headers = properties_->headers;
        if (headers.find("Host") == headers.end()) {
            request_buffer << "Host: " << parsed_url_.GetHost().to_string()
                << crlf;
        }


        if (headers.find("Content-Length") == headers.end()) {

            size_t length = 0;
            if (body_ && body_->HaveSize()) {
                length = static_cast<decltype(length)>(body_->GetFizxedSize());
            }

            request_buffer << "Content-Length: " << length << crlf;
            fixed_content_lenght_ = length;
        }

        for(const auto& it : headers) {
            request_buffer << it.first << column << it.second << crlf;
        }

        // End the header section.
        request_buffer << crlf;

        return request_buffer.str();
    }

    unique_ptr<Reply> Execute(Context& ctx) override {
        const Connection::Type protocol_type =
            (parsed_url_.GetProtocol() == Url::Protocol::HTTPS)
            ? Connection::Type::HTTPS
            : Connection::Type::HTTP;

        bytes_sent_ = 0;

        write_buffers_t write_buffer;
        ToBuffer headers(BuildOutgoingRequest());
        write_buffer.push_back(headers);
        header_size_ = boost::asio::buffer_size(write_buffer);
        std::string request_data = BuildOutgoingRequest();
        if (body_) {
            if (dirty_) {
                body_->Reset();
            }
            body_->GetData(write_buffer);
        }

        dirty_ = true;
        boost::asio::ip::tcp::resolver resolver(owner_.GetIoService());
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
        bool connected = false;

        for(; address_it != addr_end; ++address_it) {
            assert(!connected);
            const auto endpoint = address_it->endpoint();

            // Get a connection from the pool
            auto connection = owner_.GetConnectionPool().GetConnection(
                endpoint, protocol_type);

            // Connect if the connection is new.
            if (!connection->GetSocket().GetSocket().is_open()) {

                std::ostringstream msg;
                msg << "Connecting to " << endpoint << endl;
                owner_.LogDebug(msg);


                auto timer = IoTimer::Create(properties_->connectTimeoutMs,
                                                owner_.GetIoService(),
                                                connection);

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

            connected = true;

            // Send the request and all data
            while(boost::asio::buffer_size(write_buffer))
            {
                auto timer = IoTimer::Create(properties_->connectTimeoutMs,
                                                owner_.GetIoService(),
                                                connection);

                try {
                    connection->GetSocket().AsyncWrite(write_buffer,
                                                        ctx.GetYield());

                    clog << "--> Sent "
                        <<  boost::asio::buffer_size(write_buffer) << " bytes: "
                        << std::endl << "--------------- WRITE START --------------" << endl;

                    for(const auto& b : write_buffer) {

                        auto bb = boost::string_ref(
                                boost::asio::buffer_cast<const char*>(b),
                                boost::asio::buffer_size(b));

                        clog << bb;
                    }

                    clog  << std::endl
                        << "--------------- WRITE END --------------" << endl
                        << std::endl;

                    bytes_sent_ += boost::asio::buffer_size(write_buffer);

                } catch(const exception& ex) {
                    std::ostringstream msg;
                    msg << "Write failed with exception type: "
                        << typeid(ex).name()
                        << ", message: " << ex.what();
                    owner_.LogDebug(msg);
                    throw;
                }

                if (body_ && !body_->IsEof()) {
                    write_buffer.clear();
                    if (!body_->GetData(write_buffer))
                        break; // No more data
                } else {
                    break; //No more data to send
                }
            }

            // Check that we sent whatever was in the content-length header
            if (fixed_content_lenght_) {
                if (fixed_content_lenght_ != GetContentBytesSent()) {
                    std::ostringstream msg;
                    msg << "I set content-lenght header to "
                        << *fixed_content_lenght_
                        << " but sent " << GetContentBytesSent()
                        << " content bytes.";
                    owner_.LogError(msg.str());
                    throw runtime_error("Sent incorrect payload size");
                }
            }

            // Pass IO resposibility to the Reply
            auto reply = Reply::Create(connection, ctx, owner_);

            // TODO: Handle redirects
            reply->StartReceiveFromServer();

            /* Return the reply. At this time the reply headers and body
                * is returned. However, the body may or may not be
                * received.
                */

            return reply;
        }

        throw runtime_error("Failed to connect");
    }

    const std::string url_;
    const Url parsed_url_;
    const Type request_type_;
    std::unique_ptr<Body> body_;
    Properties::ptr_t properties_;
    RestClient &owner_;
    size_t header_size_ = 0;
    std::uint64_t bytes_sent_ = 0;
    boost::optional<uint64_t> fixed_content_lenght_;
    bool dirty_ = false;
};


std::unique_ptr<Request>
Request::Create(const std::string& url,
                const Type requestType,
                RestClient& owner,
                std::unique_ptr<Body> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers) {

    return make_unique<RequestImpl>(url, requestType, owner, move(body), args, headers);
}

} // restc_cpp

