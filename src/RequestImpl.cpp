#include <iostream>
#include <fstream>
#include <thread>
#include <future>
#include <array>

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

namespace {

// We support versions of boost prior to the introduction of this convenience function

boost::asio::ip::address_v6 make_address_v6(const char* str,
    boost::system::error_code& ec)
{
  boost::asio::ip::address_v6::bytes_type bytes;
  unsigned long scope_id = 0;
  if (boost::asio::detail::socket_ops::inet_pton(
        BOOST_ASIO_OS_DEF(AF_INET6), str, &bytes[0], &scope_id, ec) <= 0)
    return boost::asio::ip::address_v6();
  return boost::asio::ip::address_v6(bytes, scope_id);
}

boost::asio::ip::address_v4 make_address_v4(const char* str,
    boost::system::error_code& ec)
{
  boost::asio::ip::address_v4::bytes_type bytes;
  if (boost::asio::detail::socket_ops::inet_pton(
        BOOST_ASIO_OS_DEF(AF_INET), str, &bytes, 0, ec) <= 0)
    return boost::asio::ip::address_v4();
  return boost::asio::ip::address_v4(bytes);
}

boost::asio::ip::address make_address(const char* str,
    boost::system::error_code& ec)
{
  boost::asio::ip::address_v6 ipv6_address =
    make_address_v6(str, ec);
  if (!ec)
    return boost::asio::ip::address{ipv6_address};

  boost::asio::ip::address_v4 ipv4_address =
    make_address_v4(str, ec);
  if (!ec)
    return boost::asio::ip::address{ipv4_address};

  return boost::asio::ip::address{};
}

} // anon ns

namespace restc_cpp {

const std::string& Request::Proxy::GetName() {
    static const array<string, 3> names = {
      "NONE", "HTTP", "SOCKS5"
    };

    return names.at(static_cast<size_t>(type));
}

namespace {

constexpr char SOCKS5_VERSION = 0x05;
constexpr char SOCKS5_TCP_STREAM = 0x01;
constexpr char SOCKS5_MAX_HOSTNAME_LEN = 255;
constexpr char SOCKS5_IPV4_ADDR = 0x01;
constexpr char SOCKS5_IPV6_ADDR = 0x04;
constexpr char SOCKS5_HOSTNAME_ADDR = 0x03;

/* hostname: example.com:123                 -> "example.com", 123
 * ipv4:     1.2.3.4:123                     -> "1.2.3.4", 123
 * ipv6:     [fe80::4479:f6ff:fea3:aa23]:123 -> "fe80::4479:f6ff:fea3:aa23", 123
 */
pair<string, uint16_t> ParseAddress(const std::string addr) {
    auto pos = addr.find('['); // IPV6
    string host;
    string port;
    if (pos != string::npos) {
        auto host = addr.substr(1); // strip '['
        pos = host.find(']');
        if (pos == string::npos) {
            throw ParseException{"IPv6 address must have a ']'"};
        }
        port = host.substr(pos);
        host = host.substr(0, pos);

        if (port.size() < 3 || (host.at(1) != ':')) {
            throw ParseException{"Need `]:<port>` in "s + addr};
        }
        port = port.substr(2);
    } else {
        // IPv4 or address
        pos = addr.find(':');
        if (pos == string::npos || (addr.size() - pos) < 2) {
            throw ParseException{"Need `:<port>` in "s + addr};
        }
        port = addr.substr(pos +1);
        host = addr.substr(0, pos);
    }

    const uint16_t port_num = stoul(port);
    if (port_num == 0 || port_num > numeric_limits<uint16_t>::max()) {
        throw ParseException{"Port `:<port>` must be a valid IP port number in "s + addr};
    }

    return {host, port_num};
}

/*! Parse the address and write the socks5 connect request */
void ParseAddressIntoSocke5ConnectRequest(const std::string& addr,
                                          vector<uint8_t>& out) {

    out.push_back(SOCKS5_VERSION);
    out.push_back(SOCKS5_TCP_STREAM);
    out.push_back(0);

    string host;
    uint16_t port = 0;

    tie(host, port) = ParseAddress(addr);

    const auto final_port = htons(port);

    boost::system::error_code ec;
    const auto a = make_address(host.c_str(), ec);
    if (ec) {
        // Assume that it's a hostname.
        // TODO: Validate with a regex
        if (host.size() > SOCKS5_MAX_HOSTNAME_LEN) {
            throw ParseException{"SOCKS5 address must be <= 255 bytes"};
        }
        if (host.size() < 1) {
            throw ParseException{"SOCKS5 address must be > 1 byte"};
        }

        out.push_back(SOCKS5_HOSTNAME_ADDR);

        // Add string lenght (single byte)
        out.push_back(static_cast<uint8_t>(host.size()));

        // Copy the address, without trailing zero
        copy(host.begin(), host.end(), back_insert_iterator<vector<uint8_t>>(out));
    } else if (a.is_v4()) {
        const auto v4 = a.to_v4();
        const auto b = v4.to_bytes();
        assert(b.size() == 4);
        out.push_back(SOCKS5_IPV4_ADDR);
        copy(b.begin(), b.end(), back_insert_iterator<vector<uint8_t>>(out));
    } else if (a.is_v6()) {
        const auto v6 = a.to_v6();
        const auto b = v6.to_bytes();
        assert(b.size() == 16);
        out.push_back(SOCKS5_IPV6_ADDR);
        copy(b.begin(), b.end(), back_insert_iterator<vector<uint8_t>>(out));
    } else {
        throw ParseException{"Internal error. Failed to parse: "s + addr};
    }

    // Add 2 byte port number in network byte order
    assert(sizeof(final_port) >= 2);
    const unsigned char *p = reinterpret_cast<const unsigned char *>(&final_port);
    out.push_back(*p);
    out.push_back(*(p +1));
}

// Return 0 whene there is no more bytes to read
size_t ValidateCompleteSocks5ConnectReply(uint8_t *buf, size_t len) {
    if (len < 5) {
        throw RestcCppException{"SOCKS5 server connect reply must start at minimum 5 bytes"s};
    }

    if (buf[0] != SOCKS5_VERSION) {
        throw ProtocolException{"Wrong/unsupported SOCKS5 version in connect reply: "s
                                + to_string(buf[0])};
    }

    if (buf[1] != 0) { // Request failed
        throw ProtocolException{"Unexpected value in SOCKS5 header[1] "s
                                + to_string(buf[1])};
    }

    size_t hdr_len = 5; // Mandatory bytes

    switch(buf[3]) {
    case SOCKS5_IPV4_ADDR:
        hdr_len += 4 + 1;
        break;
    case SOCKS5_IPV6_ADDR:
        hdr_len += 16 + 1;
        break;
    case SOCKS5_HOSTNAME_ADDR:
        if (len < 4) {
            return false; // We need the length field...
        }
        hdr_len += buf[3] + 1 + 1;
    break;
    default:
        throw ProtocolException{"Wrong/unsupported SOCKS5 BINDADDR type: "s
                                + to_string(buf[3])};
    }

    if (len > hdr_len) {
        throw NotSupportedException{"SOCKS5: Received more data then the connect response."};
    }

    return hdr_len;
}

void DoSocks5Handshake(Connection& connection,
                       const Url& url,
                       const Request::Properties properties,
                       Context& ctx) {

    assert(properties.proxy.type == Request::Proxy::Type::SOCKS5);
    auto& sck = connection.GetSocket();

    // Send no-auth handshake
    {
        array<uint8_t, 3> hello = {SOCKS5_VERSION, 1, 0};
        RESTC_CPP_LOG_TRACE_("DoSocks5Handshake - saying hello");
        sck.AsyncWriteT(hello, ctx.GetYield());
    }
    {
        array<uint8_t, 2> reply = {};
        RESTC_CPP_LOG_TRACE_("DoSocks5Handshake - waiting for greeting");
        sck.AsyncRead({reply.data(), 2}, ctx.GetYield());

        if (reply[0] != SOCKS5_VERSION) {
            throw ProtocolException{"Wrong/unsupported SOCKS5 version: "s + to_string(reply[0])};
        }

        if (reply[1] != 0) {
            throw AccessDeniedException{"SOCKS5 Access denied: "s + to_string(reply[1])};
        }
    }

    // Send connect parameters
    {
        vector<uint8_t> params;

        auto addr = url.GetHost().to_string() + ":" + to_string(url.GetPort());

        ParseAddressIntoSocke5ConnectRequest(addr, params);
        RESTC_CPP_LOG_TRACE_("DoSocks5Handshake - saying connect to " <<  url.GetHost().to_string() << ":" << url.GetPort());
        sck.AsyncWriteT(params, ctx.GetYield());
    }

    {
        array<uint8_t, 255 + 6> reply;
        size_t remaining = 5; // Minimum length
        uint8_t *next = reply.data();

        RESTC_CPP_LOG_TRACE_("DoSocks5Handshake - waiting for connect confirmation - first segment");
        auto read = sck.AsyncRead({next, remaining}, ctx.GetYield());
        const auto hdr_len = ValidateCompleteSocks5ConnectReply(reply.data(), read);
        if (hdr_len > read) {
            remaining = hdr_len - read;
            next += read;
            if (hdr_len > reply.size()) {
                throw ProtocolException{"SOCKS5 Connect header from the server is too large"};
            }
            RESTC_CPP_LOG_TRACE_("DoSocks5Handshake - waiting for connect confirmation - second segment");
            read += sck.AsyncRead({next, remaining}, ctx.GetYield());
            ValidateCompleteSocks5ConnectReply(reply.data(), read);
        }

        assert(read == hdr_len);
    }
    RESTC_CPP_LOG_TRACE_("DoSocks5Handshake - done");
}
} // anonumous ns

class RequestImpl : public Request {
public:

    struct RedirectException{

        RedirectException(const RedirectException&) = delete;
        RedirectException(RedirectException &&) = default;

        RedirectException(int redirCode, string redirUrl, std::unique_ptr<Reply> reply)
        : code{redirCode}, url{move(redirUrl)}, redirectReply{move(reply)}
        {}

        RedirectException() = delete;
        ~RedirectException() = default;
        RedirectException& operator = (const RedirectException&) = delete;
        RedirectException& operator = (RedirectException&&) = delete;

        int GetCode() const noexcept { return code; };
        const std::string& GetUrl() const noexcept { return url; }
        Reply& GetRedirectReply() const { return *redirectReply; }

    private:
        const int code;
        std::string url;
        std::unique_ptr<Reply> redirectReply;
    };

    RequestImpl(std::string url,
                const Type requestType,
                RestClient& owner,
                std::unique_ptr<RequestBody> body,
                const boost::optional<args_t>& args,
                const boost::optional<headers_t>& headers,
                const boost::optional<auth_t>& auth = {})
    : url_{move(url)}, parsed_url_{url_.c_str()} , request_type_{requestType}
    , body_{move(body)}, owner_{owner}
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
        // Silence the cursed clang-tidy...
        constexpr auto magic_4 = 4;
        constexpr auto magic_6 = 6;
        constexpr auto magic_8 = 8;
        constexpr auto magic_3f = 0x3F;

        static const string alphabeth {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
        string out;

        int val = 0;
        int valb = -magic_6;
        for (const uint8_t c : in) {
            val = (val<<magic_8) + c;
            valb += magic_8;
            while (valb>=0) {
                out.push_back(alphabeth[(val>>valb)&magic_3f]);
                valb-=magic_6;
            }
        }
        if (valb>-magic_6) out.push_back(alphabeth[((val<<magic_8)>>(valb+magic_8))&magic_3f]);
        while (out.size()%magic_4) out.push_back('=');
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
        static const std::array<std::string, 7> names =
            {{ "GET", "POST", "PUT", "DELETE", "OPTIONS",
                "HEAD", "PATCH"
            }};

        return names.at(static_cast<size_t>(requestType));
    }

    uint64_t GetContentBytesSent() const noexcept {
        return bytes_sent_ - header_size_;
    }

    unique_ptr<Reply> Execute(Context& ctx) override {
        int redirects = 0;
        while(true) {
            try {
                return DoExecute((ctx));
            } catch(const RedirectException& ex) {
                
                auto url = ex.GetUrl();
                
                if (properties_->redirectFn) {
                    properties_->redirectFn(ex.GetCode(), url, ex.GetRedirectReply());
                }
                
                if ((properties_->maxRedirects >= 0)
                    && (++redirects > properties_->maxRedirects)) {
                    throw ConstraintException("Too many redirects.");
                }

                RESTC_CPP_LOG_DEBUG_("Redirecting ("
                    << ex.GetCode()
                    << ") '" << url_
                    << "' --> '"
                    << url
                    << "') ");
                url_ = move(url);
                parsed_url_ = url_.c_str();
                add_url_args_ = false; // Use whatever arguments we got in the redirect
            }
        }
    }


private:
    void ValidateReply(const Reply& reply) {
        // Silence the cursed clang tidy!
        constexpr auto magic_2 = 2;
        constexpr auto magic_100 = 100;
        constexpr auto http_401 = 401;
        constexpr auto http_403 = 403;
        constexpr auto http_404 = 404;
        constexpr auto http_405 = 405;
        constexpr auto http_406 = 406;
        constexpr auto http_407 = 407;
        constexpr auto http_408 = 408;

        const auto& response = reply.GetHttpResponse();
        if ((response.status_code / magic_100) > magic_2) switch(response.status_code) {
            case http_401:
                throw HttpAuthenticationException(response);
            case http_403:
                throw HttpForbiddenException(response);
            case http_404:
                throw HttpNotFoundException(response);
            case http_405:
                throw HttpMethodNotAllowedException(response);
            case http_406:
                throw HttpNotAcceptableException(response);
            case http_407:
                throw HttpProxyAuthenticationRequiredException(response);
            case http_408:
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
        const auto proxy_type = properties_->proxy.type;

        if (proxy_type == Request::Proxy::Type::SOCKS5) {
            string host;
            uint16_t port = 0;
            tie(host, port) = ParseAddress(properties_->proxy.address);

            RESTC_CPP_LOG_TRACE_("Using " << properties_->proxy.GetName()
                                 << " Proxy at: "
                                 << host << ':' << port);

            return {host, to_string(port)};
        }

        if (proxy_type == Request::Proxy::Type::HTTP) {
            Url proxy {properties_->proxy.address.c_str()};

            RESTC_CPP_LOG_TRACE_("Using " << properties_->proxy.GetName()
                                 << " Proxy at: "
                                 << proxy.GetHost() << ':' << proxy.GetPort());

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

    template <typename protocolT>
    boost::asio::ip::tcp::endpoint ToEp(const std::string& endp,
                                        const protocolT& protocol,
                                        Context& ctx) const {
        string host, port;

        auto endipv6 = endp.find(']');
        if (endipv6 == string::npos) {
            endipv6 = 0;
        }
        const auto pos = endp.find(':', endipv6);
        if (pos == string::npos) {
            host = endp;
        } else {
            host = endp.substr(0, pos);
            if (endp.size() > pos) {
                port = endp.substr(pos + 1);
            }
        }

        // Strip IPv6 brackets, asio can't resolve it
        if (endipv6 && !host.empty() && host.front() == '[') {
            host = host.substr(1, endipv6 -1);
        }

        RESTC_CPP_LOG_TRACE_("host=" << host << ", port=" << port);

        if (host.empty()) {
            const auto port_num = stoi(port);
            if (port_num < 1 || port_num > std::numeric_limits<uint16_t>::max()) {
                throw RestcCppException{"Port number out of range: "s + port};
            }
            return {protocol, static_cast<uint16_t>(port_num)};
        }

        boost::asio::ip::tcp::resolver::query q{host, port};
        boost::asio::ip::tcp::resolver resolver(owner_.GetIoService());

        auto ep = resolver.async_resolve(q, ctx.GetYield());
        const decltype(ep) addr_end;
        for(; ep != addr_end; ++ep)
        if (ep != addr_end) {

            RESTC_CPP_LOG_TRACE_("ep=" << ep->endpoint() << ", protocol=" << ep->endpoint().protocol().protocol());

            if (protocol == ep->endpoint().protocol()) {
                return ep->endpoint();
            }

            RESTC_CPP_LOG_TRACE_("Incorrect protocol, looping for next alternative");
        }

        RESTC_CPP_LOG_ERROR_("Failed to resolve endpoint " << endp
                             << " with protocol " << protocol.protocol());
        throw FailedToResolveEndpointException{"Failed to resolve endpoint: "s + endp};
    }


    auto GetBindProtocols(const std::string& endp, Context& ctx) {
        std::vector<decltype(boost::asio::ip::tcp::v4())> p;

        if (!endp.empty()) {
            try {
                ToEp(endp, boost::asio::ip::tcp::v4(), ctx);
                p.push_back(boost::asio::ip::tcp::v4());
            } catch (const FailedToResolveEndpointException&) {
                ;
            }

            try {
                ToEp(endp, boost::asio::ip::tcp::v6(), ctx);
                p.push_back(boost::asio::ip::tcp::v6());
            } catch (const FailedToResolveEndpointException&) {
                ;
            }
        }

        return p;
    }

    Connection::ptr_t Connect(Context& ctx) {

        static const auto timer_name = "Connect"s;

        auto prot_filter = GetBindProtocols(properties_->bindToLocalAddress, ctx);

        const Connection::Type protocol_type =
            (parsed_url_.GetProtocol() == Url::Protocol::HTTPS)
            ? Connection::Type::HTTPS
            : Connection::Type::HTTP;

        boost::asio::ip::tcp::resolver resolver(owner_.GetIoService());
        // Resolve the hostname
        const auto query = GetRequestEndpoint();

        RESTC_CPP_LOG_TRACE_("Resolving " << query.host_name() << ":"
            << query.service_name());

        auto address_it = resolver.async_resolve(query,
                                                 ctx.GetYield());
        const decltype(address_it) addr_end;

        for(; address_it != addr_end; ++address_it) {
//            if (owner_.IsClosing()) {
//                RESTC_CPP_LOG_DEBUG_("RequestImpl::Connect: The rest client is closed (at first loop). Aborting.");
//                throw FailedToConnectException("Failed to connect (closed)");
//            }

            const auto endpoint = address_it->endpoint();

            RESTC_CPP_LOG_TRACE_("Trying endpoint " << endpoint);

            for(size_t retries = 0; retries < 8; ++retries) {
                // Get a connection from the pool
                auto connection = owner_.GetConnectionPool()->GetConnection(
                    endpoint, protocol_type);

                // Connect if the connection is new.
                if (connection->GetSocket().IsOpen()) {
                    return connection;
                }

                RESTC_CPP_LOG_DEBUG_("Connecting to " << endpoint);

                if (!properties_->bindToLocalAddress.empty()) {

                    // Only connect outwards to protocols we can bind to
                    if (!prot_filter.empty()) {
                        if (std::find(prot_filter.begin(), prot_filter.end(), endpoint.protocol())
                                == prot_filter.end()) {
                            RESTC_CPP_LOG_TRACE_("Filtered out (protocol mismatch) local address: "
                                << properties_->bindToLocalAddress);
                            break; // Break out of retry loop, re-enter endpoint loop
                        }
                    }

                    RESTC_CPP_LOG_TRACE_("Binding to local address: "
                        << properties_->bindToLocalAddress);

                    boost::system::error_code ec;
                    auto local_ep = ToEp(properties_->bindToLocalAddress, endpoint.protocol(), ctx);
                    auto& sck = connection->GetSocket().GetSocket();
                    sck.open(local_ep.protocol());
                    sck.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
                    sck.bind(local_ep, ec);

                    if (ec) {
                        RESTC_CPP_LOG_ERROR_("Failed to bind to local address '"
                            << local_ep
                            << "': " << ec.message());

                        sck.close();
                        throw RestcCppException{"Failed to bind to local address: "s
                                                + properties_->bindToLocalAddress};
                    }
                }


//                if (owner_.IsClosed()) {
//                    RESTC_CPP_LOG_DEBUG_("RequestImpl::Connect: The rest client is closed. Aborting.");
//                    throw FailedToConnectException("Failed to connect (closed)");
//                }

                auto timer = IoTimer::Create(timer_name,
                    properties_->connectTimeoutMs, connection);

                try {
                    if (retries) {
                        RESTC_CPP_LOG_DEBUG_("RequestImpl::Connect: taking a nap");
                        ctx.Sleep(retries * 20ms);
                        RESTC_CPP_LOG_DEBUG_("RequestImpl::Connect: Waking up. Will try to read from the socket now.");
                    }

                    if (properties_->proxy.type == Proxy::Type::SOCKS5) {
                        connection->GetSocket().SetAfterConnectCallback([&]() {
                            RESTC_CPP_LOG_TRACE_("RequestImpl::Connect: In Socks5 callback");

                            DoSocks5Handshake(*connection, parsed_url_, *properties_, ctx);

                            RESTC_CPP_LOG_TRACE_("RequestImpl::Connect: Leaving Socks5 callback");
                        });
                    }

                    RESTC_CPP_LOG_TRACE_("RequestImpl::Connect: calling AsyncConnect --> " << endpoint);
                    connection->GetSocket().AsyncConnect(
                        endpoint, address_it->host_name(),
                        properties_->tcpNodelay, ctx.GetYield());
                    RESTC_CPP_LOG_TRACE_("RequestImpl::Connect: OK AsyncConnect --> " << endpoint);
                    return connection;
                } catch (const boost::system::system_error& ex) {
                    RESTC_CPP_LOG_TRACE_("RequestImpl::Connect:: Caught boost::system::system_error exception: \"" << ex.what()
                                         << "\". Will close connection " << *connection);
                    connection->GetSocket().GetSocket().close();

                    if (ex.code() == boost::system::errc::resource_unavailable_try_again) {
                        if ( retries < 8) {
                            RESTC_CPP_LOG_DEBUG_(
                                "RequestImpl::Connect:: Caught boost::system::system_error exception: \""
                                    << ex.what()
                                    << "\" while connecting to " << endpoint
                                    << ". I will continue the retry loop.");
                            continue;
                        }
                    }
                    RESTC_CPP_LOG_DEBUG_(
                        "RequestImpl::Connect:: Caught boost::system::system_error exception: \""
                            << ex.what()
                            << "\" while connecting to " << endpoint);
                    break; // Go to the next endpoint
                } catch(const exception& ex) {
                    RESTC_CPP_LOG_WARN_("Connect to "
                        << endpoint
                        << " failed with exception type: "
                        << typeid(ex).name()
                        << ", message: " << ex.what());

                    connection->GetSocket().GetSocket().close();
                }

            } // retries
        } // endpoints

        throw FailedToConnectException("Failed to connect (exhausted all options)");
    }

    void SendRequestPayload(Context& /*ctx*/,
                      write_buffers_t write_buffer) {

        static const auto timer_name = "SendRequestPayload"s;
        bool have_sent_headers = false;

        if (properties_->beforeWriteFn) {
            properties_->beforeWriteFn();
        }

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
                RESTC_CPP_LOG_WARN_("Write failed with exception type: "
                    << typeid(ex).name()
                    << ", message: " << ex.what());
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

        RESTC_CPP_LOG_TRACE_("Request: " << (const boost::string_ref)headers
            << ' ' << *connection_);

        PrepareBody();
        SendRequestPayload(ctx, write_buffer);
        if (properties_->afterWriteFn) {
            properties_->afterWriteFn();
        }

        RESTC_CPP_LOG_DEBUG_("Sent " << Verb(request_type_) << " request to '" << url_ << "' "
            << *connection_);

        assert(writer_);
        return *writer_;
    }

    unique_ptr<Reply> GetReply(Context& ctx) override {
        constexpr auto http_301 = 301;
        constexpr auto http_302 = 302;

        // We will not send more data regarding the current request
        writer_->Finish();
        writer_.reset();

        RESTC_CPP_LOG_TRACE_("GetReply: writer is reset.");

        DataReader::ReadConfig cfg;
        cfg.msReadTimeout = properties_->recvTimeout;
        auto reply = ReplyImpl::Create(connection_, ctx, owner_, properties_,
                                       request_type_);

        RESTC_CPP_LOG_TRACE_("GetReply: Calling StartReceiveFromServer");
        try {
            reply->StartReceiveFromServer(
                DataReader::CreateIoReader(connection_, ctx, cfg));
        } catch (const exception& ex) {
            RESTC_CPP_LOG_DEBUG_("GetReply: exception from StartReceiveFromServer: " << ex.what());
            throw;
        }

        RESTC_CPP_LOG_TRACE_("GetReply: Returned from StartReceiveFromServer. code=" << reply->GetResponseCode());

        const auto http_code = reply->GetResponseCode();
        if (http_code == http_301 || http_code == http_302) {
            auto redirect_location = reply->GetHeader("Location");
            if (!redirect_location) {
                throw ProtocolException(
                    "No Location header in redirect reply");
            }
            RESTC_CPP_LOG_TRACE_("GetReply: RedirectException. location=" << *redirect_location);
            throw RedirectException(http_code, *redirect_location, move(reply));
        }

        if (properties_->throwOnHttpError) {
            RESTC_CPP_LOG_TRACE_("GetReply: Calling ValidateReply");
            ValidateReply(*reply);
            RESTC_CPP_LOG_TRACE_("GetReply: returning from ValidateReply");
        }

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

