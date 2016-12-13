#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <bitset>

#include <boost/utility/string_ref.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/Url.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/ConnectionPool.h"
#include "restc-cpp/IoTimer.h"
#include "restc-cpp/error.h"
#include "ReplyImpl.h"

using namespace std;

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
                std::unique_ptr<Body> body,
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
        std::string pre_base = auth.name + ':' + auth.passwd;
        properties_->headers[authorization] = "Basic " + Base64Encode(pre_base);
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
                if ((redirects >= 0)
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
            }
        }
    }


private:
    void ValidateReply(const Reply& reply) {
        const auto& response = reply.GetHttpResponse();
        if (response.status_code > 299) switch(response.status_code) {
            case 401:
                throw AuthenticationException(response);
            default:
                throw RequestFailedWithErrorException(response);
        }
    }

    static std::bitset<255> GetNormalCh() {
        std::bitset<255> bits;

        for(uint8_t ch = 0; ch < bits.size(); ++ch) {
            if ((ch >= '0' && ch <= '9')
                || (ch >= 'a' && ch <= 'z')
                || (ch >= 'A' && ch <= 'Z')
                || ch == '-' || ch == '_' || ch == '.'
                || ch == '!' || ch == '~' || ch == '*'
                || ch == '\'' || ch == '(' || ch == ')'
                || ch == '/')

            {
                bits[ch] = true;
            }
        }

        return bits;
    }

    std::string UrlEncode(const boost::string_ref& src) {
        static const string hex{"0123456789ABCDEF"};
        static const std::bitset<255> normal_ch = GetNormalCh();
        std::string rval;
        rval.reserve(src.size());


        for(auto ch : src) {
            if (normal_ch[static_cast<uint8_t>(ch)]) {
                rval += ch;
            } else {
                rval += '%';
                rval += hex[(ch >> 4) & 0x0f];
                rval += hex[ch & 0x0f];
            }
        }
        return rval;
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

        request_buffer << UrlEncode(parsed_url_.GetPath());


        // Add arguments to the path as ?name=value&name=value...
        bool first_arg = true;
        for(const auto& arg : properties_->args) {
            if (first_arg) {
                first_arg = false;
                if (!parsed_url_.GetPath().ends_with('/')) {
                    request_buffer << '/';
                }
                request_buffer << '?';
            } else {
                request_buffer << '&';
            }

            request_buffer << UrlEncode(arg.name) << '=' << UrlEncode(arg.value);
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

    unique_ptr<Reply> DoExecute(Context& ctx) {
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
        const auto query = GetRequestEndpoint();

        RESTC_CPP_LOG_TRACE << "Resolving " << query.host_name() << ":"
            << query.service_name();

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
            if (!connection->GetSocket().IsOpen()) {

                RESTC_CPP_LOG_DEBUG << "Connecting to " << endpoint;

                auto timer = IoTimer::Create(properties_->connectTimeoutMs,
                                                owner_.GetIoService(),
                                                connection);

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

                    RESTC_CPP_LOG_TRACE << "--> Sent "
                        <<  boost::asio::buffer_size(write_buffer) << " bytes: "
                        << "\r\n--------------- WRITE START --------------\r\n";

                    for(const auto& b : write_buffer) {

                        auto bb = boost::string_ref(
                                boost::asio::buffer_cast<const char*>(b),
                                boost::asio::buffer_size(b));

                        RESTC_CPP_LOG_TRACE << bb;
                    }

                    RESTC_CPP_LOG_TRACE
                        << "\r\n--------------- WRITE END --------------";

                    bytes_sent_ += boost::asio::buffer_size(write_buffer);

                } catch(const exception& ex) {
                    RESTC_CPP_LOG_WARN << "Write failed with exception type: "
                        << typeid(ex).name()
                        << ", message: " << ex.what();
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
                    RESTC_CPP_LOG_ERROR << "I set content-lenght header to "
                        << *fixed_content_lenght_
                        << " but sent " << GetContentBytesSent()
                        << " content bytes.";
                    throw ProtocolException("Sent incorrect payload size");
                }
            }

            // Pass IO responsibility to the Reply
            RESTC_CPP_LOG_DEBUG << "Sending request to '" << url_ << "' "
                << *connection;
            auto reply = ReplyImpl::Create(connection, ctx, owner_);
            reply->StartReceiveFromServer(
                DataReader::CreateIoReader(*connection, ctx));

            const auto http_code = reply->GetResponseCode();
            if (http_code == 301 || http_code == 302) {
                auto redirect_location = reply->GetHeader("Location");
                if (!redirect_location) {
                    throw ProtocolException("No Location header in redirect reply");
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

        throw FailedToConnectException("Failed to connect");
    }

    std::string url_;
    Url parsed_url_;
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
                const boost::optional<headers_t>& headers,
                const boost::optional<auth_t>& auth) {

    return make_unique<RequestImpl>(url, requestType, owner, move(body), args, headers, auth);
}

} // restc_cpp

