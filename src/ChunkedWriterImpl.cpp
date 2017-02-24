
#include <array>
#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/Connection.h"
#include "restc-cpp/Socket.h"
#include "restc-cpp/DataWriter.h"
#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {


class ChunkedWriterImpl : public DataWriter {
public:
    ChunkedWriterImpl(add_header_fn_t fn, ptr_t&& source)
    : next_{move(source)},  add_header_fn_{move(fn)}
    {
    }

    void WriteDirect(boost::asio::const_buffers_1 buffers) override {
        next_->WriteDirect(buffers);
    }

    void Write(boost::asio::const_buffers_1 buffers) override {
        const auto len = boost::asio::buffer_size(buffers);
        buffers_.resize(2);
        buffers_[1] = buffers;
        DoWrite(len);
    }

    void Write(const write_buffers_t& buffers) override {
        const auto len = boost::asio::buffer_size(buffers);
        buffers_.resize(1);
        for(auto &b : buffers) {
            buffers_.push_back(b);
        }
        DoWrite(len);
    }

    void Finish() override {
        static const std::string crlfx2{"\r\n\r\n"};
        std::string finito = {"\r\n0"};

        if (add_header_fn_) {
            finito += add_header_fn_();
        }

        finito += crlfx2;

        next_->Write({finito.c_str(), finito.size()});

        next_->Finish();
    }

    void SetHeaders(Request::headers_t& headers) override {
        static const string transfer_encoding{"Transfer-Encoding"};
        static const string chunked{"chunked"};

        headers[transfer_encoding] = chunked;

        next_->SetHeaders(headers);
    }

private:
    // Set the chunk header and send the data
    void DoWrite(const size_t len) {
        if (len == 0) {
            return;
        }

        if (len > 0x7fffffff) {
            throw ConstraintException("Input buffer is too large");
        }

        // The data part of  buffers_ must be properly initialized
        assert(buffers_.size() > 1);

        int digits = 2;
        array<char, 12> header;

        if (first_) {
            digits = 0;
            first_ = false;
        } else {
            header[0] = '\r';
            header[1] = '\n';
        }

        digits += snprintf(header.data() + digits, header.size() +(-digits -2), "%x",
                               static_cast<unsigned int>(len));
        assert(digits < static_cast<int>(header.size()));
        header[digits++] = '\r';
        assert(digits < static_cast<int>(header.size()));
        header[digits++] = '\n';

        buffers_[0] = {header.data(), static_cast<size_t>(digits)};
        next_->Write(buffers_);
    }

    bool first_ = true;
    unique_ptr<DataWriter> next_;
    write_buffers_t buffers_;
    add_header_fn_t add_header_fn_;
};


DataWriter::ptr_t
DataWriter::CreateChunkedWriter(add_header_fn_t fn, ptr_t&& source) {
    return make_unique<ChunkedWriterImpl>(move(fn), move(source));
}

} // namespace



