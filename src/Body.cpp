#include <assert.h>
#include <array>

#include <boost/utility/string_ref.hpp>
#include "restc-cpp/restc-cpp.h"


using namespace std;


namespace restc_cpp {

bool Request::Body::GetData(write_buffers_t& buffers) {
    if (eof_) {
        return false;
    }
    if (type_ == Type::STRING) {
        assert(body_str_);
        buffers.push_back({body_str_->c_str(), body_str_->size()});
        eof_ = true;
        return true;
    }
    if (type_ == Type::FILE) {
        // TODO: Change to memory mapped IO
        if (!file_) {
            assert(path_);
            file_ = std::make_unique<std::ifstream>(
                path_->string(), std::ios::binary);

            buffer_ = make_unique<std::array<char, 1024 * 8>>();
        }

        const auto bytes_left = GetFizxedSize() - bytes_read_;
        if (bytes_left == 0) {
            eof_ = true;
            return false;
        }

        auto want_bytes = std::min(buffer_->size(), static_cast<size_t>(bytes_left));
        file_->read(buffer_->data(), want_bytes);
        const size_t read_this_time = static_cast<size_t>(file_->gcount());
        if (read_this_time == 0) {
            eof_ = true;
            return false;
        }

        bytes_read_ += read_this_time;
        buffers.push_back({buffer_->data(), read_this_time});
        return true;
    }
    return false;
}

std::uint64_t  Request::Body::GetFizxedSize() const {
    if (!size_) {
        if (type_ == Type::STRING) {
            assert(body_str_);
            size_ = static_cast<std::uint64_t>(body_str_->size());
        } else if (type_ == Type::FILE) {
            assert(path_);
            size_ = boost::filesystem::file_size(*path_);
        } else {
            size_ = 0;
        }
    }
    return *size_;
}

} // namespace

