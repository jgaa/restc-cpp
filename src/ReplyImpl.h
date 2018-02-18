
#include <iostream>
#include <thread>
#include <future>
#include <unordered_map>

#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/IoTimer.h"
#include "restc-cpp/DataReader.h"

using namespace std;

namespace restc_cpp {

class ReplyImpl : public Reply {
public:
    enum class ChunkedState
        { NOT_CHUNKED, GET_SIZE, IN_SEGMENT, IN_TRAILER, DONE };

    ReplyImpl(Connection::ptr_t connection, Context& ctx,
              RestClient& owner, Request::Properties::ptr_t& properties);

    ReplyImpl(Connection::ptr_t connection, Context& ctx,
              RestClient& owner);

    ~ReplyImpl();

    boost::optional<string> GetHeader(const string& name) override;
    std::deque<std::string> GetHeaders(const std::string& name) override;

    void StartReceiveFromServer(DataReader::ptr_t&& reader);

    int GetResponseCode() const override {
        return response_.status_code;
    }

    const HttpResponse& GetHttpResponse() const override {
        return response_;
    }

    boost::asio::const_buffers_1 GetSomeData() override;

    string GetBodyAsString(size_t maxSize
        = RESTC_CPP_SANE_DATA_LIMIT) override;

    bool MoreDataToRead() override {
        return !IsEof();
    }

    boost::uuids::uuid GetConnectionId() const override {
        return connection_id_;
    }

    static std::unique_ptr<ReplyImpl>
    Create(Connection::ptr_t connection,
           Context& ctx,
           RestClient& owner,
           Request::Properties::ptr_t& properties);

    static boost::string_ref b2sr(boost::asio::const_buffers_1 buffer) {
        return { boost::asio::buffer_cast<const char*>(buffer),
            boost::asio::buffer_size(buffer)};
    }

    bool IsEof() const {
        return !reader_ || reader_->IsEof();
    }


protected:
    void CheckIfWeAreDone();
    void ReleaseConnection();
    void HandleDecompression();
    void HandleContentType(std::unique_ptr<DataReaderStream>&& stream);
    void HandleConnectionLifetime();

    Connection::ptr_t connection_;
    Context& ctx_;
    Request::Properties::ptr_t properties_;
    RestClient& owner_;
    Reply::HttpResponse response_;
    headers_t headers_;
    bool do_close_connection_ = false;
    boost::optional<size_t> content_length_;
    const boost::uuids::uuid connection_id_;
    std::unique_ptr<DataReader> reader_;
};


} // restc_cpp

