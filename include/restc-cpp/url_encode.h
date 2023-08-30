#pragma once

#include "restc-cpp.h"

#include <boost/utility/string_ref.hpp>

namespace restc_cpp {

std::string url_encode(const boost::string_ref& src);

} // namespace

