#pragma once

#define RESTC_CPP_TEST_HELPER_H_

#include <cstdlib>
#include <boost/algorithm/string.hpp>

namespace restc_cpp {

#   define EXPECT_CLOSE(expect, value, slack)  EXPECT_GE(value, (expect - slack)); EXPECT_LE(value, (expect + slack));
#   define EXPECT_HTTP_OK(res) EXPECT_GE(res, 200); EXPECT_LE(res, 201)
#   define EXPECT_EQ_ENUM(a, b) EXPECT_EQ(static_cast<int>(a), static_cast<int>(b))

// Substitute localhost with whatever is in the environment-variable
// RESTC_CPP_TEST_DOCKER_ADDRESS
inline std::string GetDockerUrl(std::string url) {
    const char *docker_addr = std::getenv("RESTC_CPP_TEST_DOCKER_ADDRESS");
    if (docker_addr) {
        boost::replace_all(url, "localhost", docker_addr);
    }
    return url;
}

} // namespace

