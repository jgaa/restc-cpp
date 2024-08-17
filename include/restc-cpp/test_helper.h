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
    const char* docker_addr = nullptr;

#ifdef _WIN32
    // On Windows, use _dupenv_s to safely retrieve the environment variable
    size_t len = 0;
    errno_t err = _dupenv_s(&docker_addr, &len, "RESTC_CPP_TEST_DOCKER_ADDRESS");
    if (err != 0 || docker_addr == nullptr) {
        docker_addr = nullptr;  // Ensure docker_addr is nullptr if the variable isn't set
    }
#else
    // On Linux/macOS, use std::getenv
    docker_addr = std::getenv("RESTC_CPP_TEST_DOCKER_ADDRESS");
#endif

    if (docker_addr) {
        boost::replace_all(url, "localhost", docker_addr);
#ifdef _WIN32
        // Free the allocated memory on Windows
        free(docker_addr);
#endif
    }

    return url;
}

} // namespace

