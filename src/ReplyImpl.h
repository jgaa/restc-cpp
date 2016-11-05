
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

using namespace std;

namespace restc_cpp {

#define BYTES_AVAILABLE (read_buffer_->size() - bytes_used)

class ReplyImpl : public Reply {
public:
    using buffer_t = std::array<char, 1024 * 16>;
    enum class ChunkedState
        { NOT_CHUNKED, GET_SIZE, IN_SEGMENT, IN_TRAILER, DONE };

    ReplyImpl(Connection::ptr_t connection,
              Context& ctx,
              RestClient& owner)
    : connection_{move(connection)}, ctx_{ctx}, owner_{owner}
    , connection_id_{connection_ ? connection_->GetId()
        : boost::uuids::random_generator()()}
    {
    }

    ~ReplyImpl();

    boost::optional< string > GetHeader(const string& name) override;

    void StartReceiveFromServer() override;
    bool IsChunked() const noexcept {
        return chunked_ != ChunkedState::NOT_CHUNKED;
    }

    int GetResponseCode() const override { return status_code_; }

    boost::asio::const_buffers_1 GetSomeData() override;

    string GetBodyAsString() override;

    bool MoreDataToRead() override {
        return !body_.empty() || !have_received_all_data_;
    }

    boost::uuids::uuid GetConnectionId() const {
        return connection_id_;
    }

protected:
    void CheckIfWeAreDone();

    void ReleaseConnection();

    void ParseHeaders(bool skip_requestline = false);

    size_t ReadHeaderAndMayBeSomeMore(size_t bytes_used = 0);

    void PrepareChunkedPayload();


    /* 1) Assume that buffer points to the start of a segment.
     * 2) If we find the header, and it's complete, set
     *    body_ to the content (if any) after the header and return
     *    true. Update the state to IN_SEGMENT. Update current_chunk_len_
     * 3) On errors, throw std::runtime_error
     * 4) If the header is valid, but incomplete, return
     *    false and set the state to GET_SIZE
     * 5) If we reached the last segment and have the header
     *    and no padding data, return true. Set the state to DONE.
     *    in the buffer and return true. Set the state to IN_TRAILER
     * 6) If we reached the last segment and have a valid buffer,
     *    update body_ wit whatever remaining data there is
     *    in the buffer and return true. Set the state to IN_TRAILER
     */
    bool ProcessChunkHeader(boost::string_ref buffer);

    boost::asio::const_buffers_1 DoGetSomeChunkedData();

    boost::asio::const_buffers_1 TakeSegmentDataFromBuffer();

    // Simple non-chunked get-data
    boost::asio::const_buffers_1 DoGetSomeData();

    boost::asio::const_buffers_1
    ReadSomeData(char *ptr, size_t bytes, bool with_timer = true);

    virtual size_t AsyncReadSome(boost::asio::mutable_buffers_1 read_buffers);


    Connection::ptr_t connection_;
    Context& ctx_;
    RestClient& owner_;
    boost::string_ref buffer_; // Valid window into memory_buffer_
    boost::string_ref status_line_; // Valid only during header processing
    boost::string_ref header_; // Valid only during header processing
    boost::string_ref body_; // payload
    int status_code_ = 0;
    boost::string_ref status_message_;
    map<string, string, ciLessLibC> headers_;
    bool have_received_all_data_ = false;
    bool do_close_connection_ = false;
    std::unique_ptr<buffer_t> read_buffer_;
    boost::optional<size_t> content_length_;
    size_t current_chunk_len_ = 0;
    size_t current_chunk_read_ = 0;
    size_t current_chunk_ = 0;
    ChunkedState chunked_ = ChunkedState::NOT_CHUNKED;
    size_t data_bytes_received_ = 0;
    size_t body_bytes_received_ = 0;
    const boost::uuids::uuid connection_id_;
};


} // restc_cpp

