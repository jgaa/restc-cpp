#include "restc-cpp/restc-cpp.h"
#include "restc-cpp/DataReader.h"

using namespace std;

namespace restc_cpp {


class NoBodyReaderImpl : public DataReader {
public:
    NoBodyReaderImpl() = default;

    [[nodiscard]] bool IsEof() const override { return true; }

    void Finish() override {
    }

    ::restc_cpp::boost_const_buffer ReadSome() override {
        return {nullptr, 0};
    }
};

DataReader::ptr_t
DataReader::CreateNoBodyReader() {
    return make_unique<NoBodyReaderImpl>();
}


} // namespace

