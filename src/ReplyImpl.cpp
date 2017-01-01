
#include<boost/tokenizer.hpp>

#include "restc-cpp/logging.h"
#include "restc-cpp/helper.h"
#include "restc-cpp/error.h"
#include "ReplyImpl.h"

using namespace std;

namespace restc_cpp {


boost::optional< string > ReplyImpl::GetHeader(const string& name) {
    boost::optional< string > rval;

    auto it = headers_.find(name);
    if (it != headers_.end()) {
        rval = it->second;
    }

    return rval;
}

ReplyImpl::ReplyImpl(Connection::ptr_t connection,
                     Context& ctx,
                     RestClient& owner)
: connection_{move(connection)}, ctx_{ctx}, owner_{owner}
, connection_id_(connection_ ? connection_->GetId()
    : boost::uuids::random_generator()())
{
}

ReplyImpl::~ReplyImpl() {
    if (connection_ && connection_->GetSocket().IsOpen()) {
        try {
            RESTC_CPP_LOG_TRACE << "~ReplyImpl(): " << *connection_
                << " is still open. Closing it to prevent problems with partially "
                << "received data.";
            connection_->GetSocket().Close();
            connection_.reset();
        } catch(std::exception& ex) {
            RESTC_CPP_LOG_WARN << "~ReplyImpl(): Caught exception:" << ex.what();
        }
    }
}

void ReplyImpl::StartReceiveFromServer(DataReader::ptr_t&& reader) {
    static const std::string content_len_name{"Content-Length"};
    static const std::string connection_name{"Connection"};
    static const std::string keep_alive_name{"keep-alive"};
    static const std::string transfer_encoding_name{"Transfer-Encoding"};
    static const std::string chunked_name{"chunked"};

    if (reader_) {
        throw RestcCppException("StartReceiveFromServer() is already called.");
    }

    assert(reader);
    auto stream = make_unique<DataReaderStream>(move(reader));
    stream->ReadServerResponse(response_);
    stream->ReadHeaderLines([this](std::string&& name, std::string&& value) {
        headers_[move(name)] = move(value);
    });
    reader_ = move(stream);

    if (const auto cl = GetHeader(content_len_name)) {
        content_length_ = stoi(*cl);
        reader_ = DataReader::CreatePlainReader(*content_length_, move(reader_));
    } else {
        auto te = GetHeader(transfer_encoding_name);
        if (te && boost::iequals(*te, chunked_name)) {
            reader_ = DataReader::CreateChunkedReader([this](string&& name, string&& value) {
                headers_[move(name)] = move(value);
            },  move(reader_));
        } else {
            reader_ = DataReader::CreateNoBodyReader();
        }
    }

    // Check for Connection: close header and tag the connection for close
    const auto conn_hdr = GetHeader(connection_name);
    if (!conn_hdr || !ciEqLibC()(*conn_hdr, keep_alive_name)) {
        RESTC_CPP_LOG_TRACE << "No 'Connection: keep-alive' header. "
            << "Tagging " << *connection_ << " for close.";
        do_close_connection_ = true;
    }

    HandleDecompression();
    CheckIfWeAreDone();
}

void ReplyImpl::HandleDecompression() {
    static const std::string content_encoding{"Content-Encoding"};
    static const std::string gzip{"gzip"};
    static const std::string deflate{"deflate"};

    const auto te_hdr = GetHeader(content_encoding);
    if (!te_hdr) {
        return;
    }

    boost::tokenizer<> tok(*te_hdr);
    for(auto it = tok.begin(); it != tok.end(); ++it) {
        if (ciEqLibC()(gzip, *it)) {
            RESTC_CPP_LOG_TRACE << "Adding gzip reader to " << *connection_;
            reader_ = DataReader::CreateGzipReader(move(reader_));
        } else if (ciEqLibC()(deflate, *it)) {
            RESTC_CPP_LOG_TRACE << "Adding deflate reader to " << *connection_;
            reader_ = DataReader::CreateZipReader(move(reader_));
        } else {
            RESTC_CPP_LOG_ERROR << "Unsupported compression: '" << *it
                << "' from server on " << *connection_;
            throw NotSupportedException("Unsupported compression.");
        }
    }
}

boost::asio::const_buffers_1 ReplyImpl::GetSomeData()  {
    auto rval = reader_->ReadSome();
    CheckIfWeAreDone();
    return rval;
}

string ReplyImpl::GetBodyAsString() {
    std::string buffer;
    if (content_length_) {
        buffer.reserve(*content_length_);
    }

    while(!reader_->IsEof()) {
        auto data = reader_->ReadSome();
        buffer.append(boost::asio::buffer_cast<const char*>(data),
                      boost::asio::buffer_size(data));
    }

    ReleaseConnection();
    return buffer;
}

void ReplyImpl::CheckIfWeAreDone() {
    if (reader_->IsEof()) {
        ReleaseConnection();
    }
}

void ReplyImpl::ReleaseConnection() {
    if (connection_ && do_close_connection_) {
        RESTC_CPP_LOG_TRACE << "Closing connection because do_close_connection_ is true: "
            << *connection_;
        if (connection_->GetSocket().IsOpen()) {
            connection_->GetSocket().Close();
        }
    }

    connection_.reset();
}

std::unique_ptr<ReplyImpl>
ReplyImpl::Create(Connection::ptr_t connection,
       Context& ctx,
       RestClient& owner) {

    return make_unique<ReplyImpl>(move(connection), ctx, owner);
}

} // restc_cpp
