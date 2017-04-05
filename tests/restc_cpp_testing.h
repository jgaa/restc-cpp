#pragma once

#include <cstdlib>
#include <boost/algorithm/string.hpp>

namespace restc_cpp {

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
