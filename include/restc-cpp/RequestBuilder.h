#pragma once
#ifndef RESTC_CPP_REQUEST_BUILDER_H_
#define RESTC_CPP_REQUEST_BUILDER_H_

#ifndef RESTC_CPP_H_
#       error "Include restc-cpp.h first"
#endif

#include <string>
#include <assert.h>

#include "restc-cpp/SerializeJson.h"
//#include "restc-cpp/DataWriter.h"
#include "restc-cpp/RequestBody.h"
#include "restc-cpp/RequestBodyWriter.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// If enabled, 'localhost' is mapped to the content of the
// environment variable RESTC_CPP_TEST_DOCKER_ADDRESS

namespace restc_cpp {

#ifdef RESTC_CPP_ENABLE_URL_TEST_MAPPING
#   ifndef RESTC_CPP_TEST_HELPER_H_
#       error "Include "restc-cpp/test_helper.h" first"
#   endif
#   define MAP_URL_FOR_TESTING(url) url = GetDockerUrl(url);
#else
#   define MAP_URL_FOR_TESTING(url)
#endif


/*! Convenience class for building requests */
class RequestBuilder
{
public:

    RequestBuilder() = default;
    RequestBuilder(const RequestBuilder&) = delete;
    RequestBuilder(RequestBuilder&&) = delete;

    void operator = (const RequestBuilder&) = delete;
    void operator = (RequestBuilder&&) = delete;

    RequestBuilder(Context& ctx)
    : ctx_{&ctx} {}

    /*! Make a HTTP GET request */
    RequestBuilder& Get(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::GET;
        return *this;
    }

    /*! Make a HTTP POST request */
    RequestBuilder& Post(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::POST;
        return *this;
    }

    /*! Make a HTTP PUT request */
    RequestBuilder& Put(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::PUT;
        return *this;
    }

    /*! Make a HTTP DELETE request */
    RequestBuilder& Delete(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::DELETE;
        return *this;
    }

    /*! Make a HTTP OPTIONS request */
    RequestBuilder& Options(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::OPTIONS;
        return *this;
    }

    /*! Make a HTTP HEAD request */
    RequestBuilder& Head(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::HEAD;
        return *this;
    }

    /*! Make a HTTP PATCH request */
    RequestBuilder& Patch(std::string url) {
        assert(url_.empty());
        url_ = std::move(url);
        MAP_URL_FOR_TESTING(url_);
        type_ = Request::Type::PATCH;
        return *this;
    }

     /*! Set a header
     *
     * \param name Name of the header
     * \param value Value of the header
     *
     * This method will overwrite any existing header
     * with the same name
     */
    RequestBuilder& Header(std::string name,
                           std::string value) {
        if (!headers_) {
            headers_ = Request::headers_t();
        }

        (*headers_)[name] = std::move(value);
        return *this;
    }

    /*! Insert a header
     *
     * \param name Name of the header
     * \param value Value of the header
     *
     * This method allows more than one header with the same name.
     */
    RequestBuilder& AddHeader(std::string name,
                           std::string value) {
        if (!headers_) {
            headers_ = Request::headers_t();
        }

        headers_->insert({std::move(name), std::move(value)});
        return *this;
    }

    /*! Add headers
     *
     * This method copies the headers. Existing headers in the destination
     * are not deleted, so if uses carelessly, you may end up with
     * duplicate headers for headers that needs to be unique.
     */
    RequestBuilder& AddHeaders(const Request::headers_t& headers) {
        if (!headers_) {
            headers_ = Request::headers_t();
        }

        for(const auto& hdr: headers) {
            headers_->insert(hdr);
        }
        return *this;
    }

     /*! Add headers
     *
     * This method copies the headers. Existing headers in the destination
     * are not deleted, so if uses carelessly, you may end up with
     * duplicate headers for headers that needs to be unique.
     */
    RequestBuilder& AddHeaders(
        boost::optional<Request::headers_t> headers) {
        if (headers) {
            return AddHeaders(*headers);
        }
        return *this;
    }


    /*! Add a request argument
     *
     * This is a URL argument that will be appended to the
     * url (like http://example.com?name=value
     * where both the name and the value will be correctly
     * url encoded.
     *
     * \param name Name of the argument
     * \param value Value of the argument
     */
    RequestBuilder& Argument(std::string name,
                             std::string value) {
        if (!args_) {
            args_ = Request::args_t();
        }

        args_->push_back({move(name), move(value)});
        return *this;
    }

    /*! Add a request argument
     *
     * This is a URL argument that will be appended to the
     * url (like http://example.com?name=value
     * where both the name and the value will be correctly
     * url encoded.
     *
     * \param name Name of the argument
     * \param value Value of the argument
     */
    RequestBuilder& Argument(std::string name, int64_t value) {
        return Argument(move(name), std::to_string(value));
    }

    /*! Supply your own RequestBody to the request
     */
    RequestBuilder& Body(std::unique_ptr<RequestBody> body) {
        assert(!body_);
        body_ = move(body);
        return *this;
    }

     /*! Body (data) for the request
     *
     * \body A text string to send as the body.
     */
    RequestBuilder& Data(const std::string& body) {
        assert(!body_);
        body_ = RequestBody::CreateStringBody(body);
        return *this;
    }

    /*! Body (data) for the request
     *
     * \body A c text string to send as the body.
     */
    RequestBuilder& Data(const char *body) {
        assert(!body_);
        assert(body);
        body_ = RequestBody::CreateStringBody(std::string(body));
        return *this;
    }

    /*! Body (data) for the request
     *
     * \body A c text string to send as the body.
     * \len Number of bytes to send.
     */
    RequestBuilder& Data(const char *body, const size_t len) {
        assert(!body_);
        assert(body);
        body_ = RequestBody::CreateStringBody(std::string(body,len));
        return *this;
    }

    /*! Body (data) for the request
     *
     * \body A text string to send as the body.
     */
    RequestBuilder& Data(std::string&& body) {
        assert(!body_);
        body_ = RequestBody::CreateStringBody(move(body));
        return *this;
    }

    /*! Use a functor to supply the data
     *
     */
    template <typename fnT>
    RequestBuilder& DataProvider(const fnT& fn) {
        assert(!body_);
        body_ = std::make_unique<RequestBodyWriter<fnT>>(fn);
        return *this;
    }

    /*! Body (file) to send as the body
     *
     * This will read the content of the file in binary mode
     * and use that as the body of the request, with no conversions
     * and no mime/field encoding.
     *
     * \param path Path to a file to upload
     */
    RequestBuilder& File(const boost::filesystem::path& path) {
        assert(!body_);
        body_ = RequestBody::CreateFileBody(path);
        return *this;
    }

    /*! Disable compression */
    RequestBuilder& DisableCompression() {
        assert(!body_);
        disable_compression_ = true;
        return *this;
    }

    /*! Supply credentials for HTTP Basic Authentication
     *
     * \param name Name to use
     * \passwd Password to use
     *
     * \Note The credentials are sent unencrypted (That's
     *      how HTTP Basic Authentication works).
     *      You should therefore only use this on internal
     *      networks or when using https:// connections.
     */
    RequestBuilder& BasicAuthentication(const std::string name,
                                        const std::string passwd) {
        assert(!body_);

        auth_ = Request::auth_t(name, passwd);
        return *this;
    }

    /*! Serialize a C++ object to Json and send it as the body.
     *
     * \param data A C++ object that is declared with
     *      BOOST_FUSION_ADAPT_STRUCT. The object will be
     *      serialized to a Json object and sent as the
     *      body of the request.
     * \param mapping Mapping between native C++ and Json property names
     * \param excludedNames Properties to ignore during serialization
     *      (typically read-only properties that will be rejected by the
     *      server).
     *
     * Normally used with POST or PUT requests.
     */
    template<typename T>
    RequestBuilder& Data(const T& data,
                         const JsonFieldMapping *mapping = nullptr,
                         const excluded_names_t *excludedNames = nullptr) {
        assert(!body_);

        auto fn = [&data, mapping, excludedNames](DataWriter& writer) {
            RapidJsonInserter<T> inserter(writer);
            if (mapping) {
                inserter.SetNameMapping(mapping);
            }
            if (excludedNames) {
                inserter.ExcludeNames(excludedNames);
            }
            inserter.Add(data);
            inserter.Done();
        };

        body_ = std::make_unique<RequestBodyWriter<decltype(fn)>>(fn);
        return *this;
    }

    /*! We will use a Chunked request body */
    RequestBuilder& Chunked() {
        static const std::string transfer_encoding{"Transfer-Encoding"};
        static const std::string chunked{"chunked"};
        return Header(transfer_encoding, chunked);
    }

    std::unique_ptr<Request> Build() {
        assert(ctx_);
        static const std::string accept_encoding{"Accept-Encoding"};
        static const std::string gzip{"gzip"};
#ifdef DEBUG
        assert(!built_);
        built_ = true;
#endif
#if RESTC_CPP_WITH_ZLIB
        if (!disable_compression_) {
            if (!headers_ || (headers_->find(accept_encoding) == headers_->end())) {
                Header(accept_encoding, gzip);
            }
        }
#endif
        return Request::Create(
            url_, type_, ctx_->GetClient(), move(body_), args_, headers_, auth_);
    }

    /*! Exceute the request.
     *
     * \returns A unique pointer to a Reply instance.
     */
    std::unique_ptr<Reply> Execute() {
        assert(ctx_);
        auto request = Build();
        return request->Execute(*ctx_);
    }

    /*! Get the body of the request.
     *
     * The function will only be able to return a value when
     * the data is supplied as a std::string or C string.
     *
     * The body is moved to the request when Build() is called,
     * so method is mostly useful for unit testing.
     */
    std::string GetData() const {
        if (body_) {
            return body_->GetCopyOfData();
        }
        return {};
    }

private:
    Context *ctx_ = nullptr;
    std::string url_;
    Request::Type type_;
    boost::optional<Request::headers_t> headers_;
    boost::optional<Request::args_t> args_;
    boost::optional<Request::auth_t> auth_;
    std::unique_ptr<RequestBody> body_;
    bool disable_compression_ = false;
#ifdef DEBUG
    bool built_ = false;
#endif
};


} // restc_cpp

#endif // RESTC_CPP_REQUEST_BUILDER_H_

