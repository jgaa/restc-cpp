#include <iostream>
#include <fstream>
#include <thread>
#include <future>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/Url.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/IoTimer.h"
#include "restc-cpp/error.h"
#include "restc-cpp/url_encode.h"
#include "restc-cpp/RequestBody.h"
#include "ReplyImpl.h"

using namespace std;
using namespace std::string_literals;

namespace restc_cpp {

class RequestImpl : public Request {
public:

    struct RedirectException{

        RedirectException(const RedirectException&) = default;
        RedirectException(RedirectException &&) = default;

        RedirectException(int redirCode, string redirUrl)
        : code{redirCode}, url{move(redirUrl)} {}

        const int code;
        std::string url;
    };

    RequestImpl(const std::string& url,
                const Type requestType,
                RestClient& owner,
                std::unique_ptr<RequestBody> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers,
                const boost::optional<auth_t>& auth = {})
    : url_{url}, parsed_url_{url_.c_str()} , request_type_{requestType}
    , body_{std::move(body)}, owner_{owner}
    {
       if (args || headers || auth) {
            Properties::ptr_t props = owner_.GetConnectionProperties();
            assert(props);
            properties_ = make_shared<Properties>(*props);

            if (args) {
                properties_->args.insert(properties_->args.end(),
                                         args->begin(), args->end());
            }

            merge_map(headers, properties_->headers);

            if (auth) {
                SetAuth(*auth);
            }

        } else {
            properties_ = owner_.GetConnectionProperties();
        }
    }

    // modified from http://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
    static std::string Base64Encode(const std::string &in) {
        static const string alphabeth {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
        string out;

        int val = 0, valb = -6;
        for (const uint8_t c : in) {
            val = (val<<8) + c;
            valb += 8;
            while (valb>=0) {
                out.push_back(alphabeth[(val>>valb)&0x3F]);
                valb-=6;
            }
        }
        if (valb>-6) out.push_back(alphabeth[((val<<8)>>(valb+8))&0x3F]);
        while (out.size()%4) out.push_back('=');
        return out;
    }

    void SetAuth(const Auth& auth) {
        static const string authorization{"Authorization"};
        static const string basic_sp{"Basic "};

        std::string pre_base = auth.name + ':' + auth.passwd;
        properties_->headers[authorization]
            = basic_sp + Base64Encode(pre_base);
        std::memset(&pre_base[0], 0, pre_base.capacity());
        pre_base.clear();
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


    unique_ptr<Reply> Execute(Context& ctx) override {
        int redirects = 0;
        while(true) {
            try {
                return DoExecute((ctx));
            } catch(RedirectException& ex) {
                if ((properties_->maxRedirects >= 0)
                    && (++redirects > properties_->maxRedirects)) {
                    throw ConstraintException("Too many redirects.");
                }

                RESTC_CPP_LOG_DEBUG << "Redirecting ("
                    << ex.code
                    << ") '" << url_
                    << "' --> '"
                    << ex.url
                    << "') ";
                url_ = move(ex.url);
                parsed_url_ = url_.c_str();
                add_url_args_ = false; // Use whatever arguments we got in the redirect
            }
        }
    }


private:
    void ValidateReply(const Reply& reply) {
        const auto& response = reply.GetHttpResponse();
        if (response.status_code > 299) switch(response.status_code) {
            case 401:
                throw HttpAuthenticationException(response);
            case 403:
                throw HttpForbiddenException(response);
            case 404:
                throw HttpNotFoundException(response);
            case 405:
                throw HttpMethodNotAllowedException(response);
            case 406:
                throw HttpNotAcceptableException(response);
            case 407:
                throw HttpProxyAuthenticationRequiredException(response);
            case 408:
                throw HttpRequestTimeOutException(response);
            default:
                throw RequestFailedWithErrorException(response);
        }
    }

    std::string BuildOutgoingRequest() {
        static const std::string crlf{"\r\n"};
        static const std::string column{": "};
        static const string host{"Host"};

        std::ostringstream request_buffer;

        // Build the request-path
        request_buffer << Verb(request_type_) << ' ';

        if (properties_->proxy.type == Request::Proxy::Type::HTTP) {
            request_buffer << parsed_url_.GetProtocolName() << parsed_url_.GetHost();
        }

        // Add arguments to the path as ?name=value&name=value...
        bool first_arg = true;
        if (add_url_args_) {
            // Normal processing.
            request_buffer << url_encode(parsed_url_.GetPath());
            for(const auto& arg : properties_->args) {
                if (first_arg) {
                    first_arg = false;
                    request_buffer << '?';
                } else {
                    request_buffer << '&';
                }

                request_buffer
                    << url_encode(arg.name)
                    << '=' << url_encode(arg.value);
            }
        } else {
            // After a redirect. We The redirect-url in parsed_url_ should be encoded,
            // and may be exactly what the target expects - so we do nothing here.
            request_buffer << parsed_url_.GetPath();
        }

        request_buffer << " HTTP/1.1" << crlf;

        // Build the header buffers
        headers_t headers = properties_->headers;
        assert(writer_);

        // Let the writers set their individual headers.
        writer_->SetHeaders(headers);

        if (headers.find(host) == headers.end()) {
            request_buffer << host << ": " << parsed_url_.GetHost().to_string() << crlf;
        }

        for(const auto& it : headers) {
            request_buffer << it.first << column << it.second << crlf;
        }

        // End the header section.
        request_buffer << crlf;

        return request_buffer.str();
    }

    boost::asio::ip::tcp::resolver::query GetRequestEndpoint() {
        if (properties_->proxy.type == Request::Proxy::Type::HTTP) {
            Url proxy {properties_->proxy.address.c_str()};

            RESTC_CPP_LOG_TRACE << "Using HTTP Proxy at: "
                << proxy.GetHost() << ':' << proxy.GetPort();

            return { proxy.GetHost().to_string(),
                proxy.GetPort().to_string()};
        }

        return { parsed_url_.GetHost().to_string(),
            parsed_url_.GetPort().to_string()};
    }

    /* If we are redirected, we need to reset the body
     * Some body implementations may not support that and throw in Reset()
     */
    void PrepareBody() {
        if (body_) {
            if (dirty_) {
                body_->Reset();
            }
        }
        dirty_ = true;
    }

    Connection::ptr_t Connect(Context& ctx) {

        static const auto timer_name = "Connect"s;

        const Connection::Type protocol_type =
            (parsed_url_.GetProtocol() == Url::Protocol::HTTPS)
            ? Connection::Type::HTTPS
            : Connection::Type::HTTP;

        boost::asio::ip::tcp::resolver resolver(owner_.GetIoService());
        // Resolve the hostname
        const auto query = GetRequestEndpoint();

        RESTC_CPP_LOG_TRACE << "Resolving " << query.host_name() << ":"
            << query.service_name();

        auto address_it = resolver.async_resolve(query,
                                                 ctx.GetYield());
        const decltype(address_it) addr_end;

        for(; address_it != addr_end; ++address_it) {
            const auto endpoint = address_it->endpoint();

            RESTC_CPP_LOG_TRACE << "Trying endpoint " << endpoint;

            // Get a connection from the pool
            auto connection = owner_.GetConnectionPool()->GetConnection(
                endpoint, protocol_type);

            // Connect if the connection is new.
            if (!connection->GetSocket().IsOpen()) {

                RESTC_CPP_LOG_DEBUG << "Connecting to " << endpoint;

                auto timer = IoTimer::Create(timer_name,
                    properties_->connectTimeoutMs, connection);

                try {
                    connection->GetSocket().AsyncConnect(
                        endpoint, ctx.GetYield());
                } catch(const exception& ex) {
                    RESTC_CPP_LOG_WARN << "Connect to "
                        << endpoint
                        << " failed with exception type: "
                        << typeid(ex).name()
                        << ", message: " << ex.what();

                    connection->GetSocket().GetSocket().close();
                    continue;
                }
            }

            return connection;
        }

        throw FailedToConnectException("Failed to connect");
    }

    void SendRequestPayload(Context& ctx,
                      write_buffers_t write_buffer) {

        static const auto timer_name = "SendRequestPayload"s;
        bool have_sent_headers = false;

        while(boost::asio::buffer_size(write_buffer))
        {
            auto timer = IoTimer::Create(timer_name,
                properties_->sendTimeoutMs, connection_);

            try {

                if (!have_sent_headers) {

                    auto b = write_buffer[0];

                    writer_->WriteDirect(
                        {boost::asio::buffer_cast<const char *>(b),
                        boost::asio::buffer_size(b)});

                    have_sent_headers = true;

                    if (write_buffer.size() > 1) {
                        write_buffer.erase(write_buffer.begin());
                        writer_->Write(write_buffer);
                    }

                } else {
                    writer_->Write(write_buffer);
                }

                bytes_sent_ += boost::asio::buffer_size(write_buffer);

            } catch(const exception& ex) {
                RESTC_CPP_LOG_WARN << "Write failed with exception type: "
                    << typeid(ex).name()
                    << ", message: " << ex.what();
                throw;
            }

            write_buffer.clear();

            if (body_) {
                switch(body_->GetType()) {
                    case RequestBody::Type::FIXED_SIZE:
                    case RequestBody::Type::CHUNKED_LAZY_PULL:
                        if (!body_->GetData(write_buffer)) {
                            return;
                        }
                        break;
                    case RequestBody::Type::CHUNKED_LAZY_PUSH:
                        body_->PushData(*writer_);
                }
            } else {
                return; // No more data to send
            }
        }
    }

    DataWriter& SendRequest(Context& ctx) override {
        bytes_sent_ = 0;

        connection_ = Connect(ctx);
        DataWriter::WriteConfig cfg;
        cfg.msWriteTimeout = properties_->sendTimeoutMs;
        writer_ = DataWriter::CreateIoWriter(connection_, ctx, cfg);

        if (body_) {
            if (body_->GetType() == RequestBody::Type::FIXED_SIZE) {
                writer_ = DataWriter::CreatePlainWriter(
                    body_->GetFixedSize(), move(writer_));
            } else {
                writer_ = DataWriter::CreateChunkedWriter(nullptr, move(writer_));
            }
        } else {
            static const string transfer_encoding{"Transfer-Encoding"};
            static const string chunked{"chunked"};
            auto h = properties_->headers.find(transfer_encoding);
            if ((h != properties_->headers.end()) && ciEqLibC()(h->second, chunked)) {
                writer_ = DataWriter::CreateChunkedWriter(nullptr, move(writer_));
            } else {
                writer_ = DataWriter::CreatePlainWriter(0, move(writer_));
            }
        }

        // TODO: Add compression

        write_buffers_t write_buffer;
        ToBuffer headers(BuildOutgoingRequest());
        write_buffer.push_back(headers);
        header_size_ = boost::asio::buffer_size(write_buffer);

        RESTC_CPP_LOG_TRACE << "Request: " << (const boost::string_ref)headers
            << ' ' << *connection_;

        PrepareBody();
        SendRequestPayload(ctx, write_buffer);

        RESTC_CPP_LOG_DEBUG << "Sent " << Verb(request_type_) << " request to '" << url_ << "' "
            << *connection_;

        assert(writer_);
        return *writer_;
    }

    unique_ptr<Reply> GetReply(Context& ctx) override {

        // We will not send more data regarding the current request
        writer_->Finish();
        writer_.reset();

        DataReader::ReadConfig cfg;
        cfg.msReadTimeout = properties_->recvTimeout;
        auto reply = ReplyImpl::Create(connection_, ctx, owner_, properties_);
        reply->StartReceiveFromServer(
            DataReader::CreateIoReader(connection_, ctx, cfg));

        const auto http_code = reply->GetResponseCode();
        if (http_code == 301 || http_code == 302) {
            auto redirect_location = reply->GetHeader("Location");
            if (!redirect_location) {
                throw ProtocolException(
                    "No Location header in redirect reply");
            }
            throw RedirectException(http_code, *redirect_location);
        }

        ValidateReply(*reply);

        /* Return the reply. At this time the reply headers and body
            * is returned. However, the body may or may not be
            * received.
            */

        return move(reply);
    }



    unique_ptr<Reply> DoExecute(Context& ctx) {
        SendRequest(ctx);
        return GetReply(ctx);
    }

    std::string url_;
    Url parsed_url_;
    const Type request_type_;
    std::unique_ptr<RequestBody> body_;
    Connection::ptr_t connection_;
    std::unique_ptr<DataWriter> writer_;
    Properties::ptr_t properties_;
    RestClient &owner_;
    size_t header_size_ = 0;
    std::uint64_t bytes_sent_ = 0;
    bool dirty_ = false;
    bool add_url_args_ = true;
};


std::unique_ptr<Request>
Request::Create(const std::string& url,
                const Type requestType,
                RestClient& owner,
                std::unique_ptr<RequestBody> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers,
                const boost::optional<auth_t>& auth) {

    return make_unique<RequestImpl>(url, requestType, owner, move(body), args, headers, auth);
}

} // restc_cpp

