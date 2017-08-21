#pragma once

#define RESTC_CPP_TEST_HELPER_H_

#include <cstdlib>
#include <boost/algorithm/string.hpp>


namespace restc_cpp {
namespace {

#define STARTCASE(name) { CASE(#name) { \
    RESTC_CPP_LOG_DEBUG << "================================"; \
    RESTC_CPP_LOG_INFO << "Test case: " << #name; \
    RESTC_CPP_LOG_DEBUG << "================================";

#define ENDCASE \
    RESTC_CPP_LOG_DEBUG << "============== ENDCASE ============="; \
}},

template<typename T1, typename T2>
bool compare(const T1& left, const T2& right) {
    const auto state = (left == right);
    if (!state) {
        std::cerr << ">>>> '" << left << "' is not equal to '" << right << "'" << std::endl;
    }
    return state;
}
} // anonymous namespace

template<typename T>
bool CHECK_CLOSE(const T expect, const T value, const T slack) {
    return (value >= (expect - slack))
        && (value <= (expect + slack));
}

// Substiture localhost with whatever is in the environment-variable
// RESTC_CPP_TEST_DOCKER_ADDRESS
inline std::string GetDockerUrl(std::string url) {
    const char *docker_addr = std::getenv("RESTC_CPP_TEST_DOCKER_ADDRESS");
    if (docker_addr) {
        boost::replace_all(url, "localhost", docker_addr);
    }
    return url;
}

} // namespace


#define CHECK_EQUAL(a,b) EXPECT(compare(a,b))
#define CHECK_EQUAL_ENUM(a,b) EXPECT(compare(static_cast<int>(a), static_cast<int>(b)))
#define TEST(name) CASE(#name)

