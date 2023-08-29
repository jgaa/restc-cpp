#pragma once

#include "restc-cpp.h"

#include <boost/utility/string_view.hpp>

namespace restc_cpp {

std::string url_encode(const boost::string_view& src);

} // namespace

