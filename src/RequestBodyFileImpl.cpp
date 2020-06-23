#include <cassert>
#include <array>

#include <boost/utility/string_ref.hpp>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/logging.h"
#include "restc-cpp/RequestBody.h"
#include "restc-cpp/DataWriter.h"

using namespace std;


namespace restc_cpp {
namespace impl {

// TODO: Change to memory mapped IO

class RequestBodyFileImpl : public RequestBody
{
public:
    RequestBodyFileImpl(boost::filesystem::path path)
    : path_{move(path)}
    , size_{boost::filesystem::file_size(path_)}
    {
        file_ = make_unique<ifstream>(path_.string(), ios::binary);
    }

    Type GetType() const noexcept override {
        return Type::FIXED_SIZE;
    }

    uint64_t GetFixedSize() const override {
        return size_;
    }

    bool GetData(write_buffers_t & buffers) override {
        const auto bytes_left = size_ - bytes_read_;
        if (bytes_left == 0) {
            eof_ = true;
            file_.reset();
            RESTC_CPP_LOG_DEBUG_("Successfully uploaded file "
                << path_
                << " of size " << size_ << " bytes.");
            return false;
        }

        auto want_bytes = min(buffer_.size(), static_cast<size_t>(bytes_left));
        file_->read(buffer_.data(), want_bytes);
        const auto read_this_time = static_cast<size_t>(file_->gcount());
        if (read_this_time == 0) {
            const auto err = errno;
            throw IoException(string{"file read failed: "}
                + to_string(err) + " " + strerror(err));
        }

        bytes_read_ += read_this_time;
        buffers.push_back({buffer_.data(), read_this_time});
        return true;
    }

    void Reset() override {
        eof_ = false;
        bytes_read_ = 0;
        file_->clear();
        file_->seekg(0);
    }

private:
    bool eof_ = false;
    boost::filesystem::path path_;
    unique_ptr<ifstream> file_;
    uint64_t bytes_read_ = 0;
    const uint64_t size_;
    static constexpr size_t buffer_size_ = 1024 * 8;
    array<char, buffer_size_> buffer_ = {};
};


} // impl

unique_ptr<RequestBody> RequestBody::CreateFileBody(
    boost::filesystem::path path) {

    return make_unique<impl::RequestBodyFileImpl>(move(path));
}

} // restc_cpp


