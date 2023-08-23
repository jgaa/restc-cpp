#pragma once

#ifndef RESTC_CPP_URL_ENCODE_H_
#define RESTC_CPP_URL_ENCODE_H_

#include "restc-cpp.h"

#include <boost/utility/string_view.hpp>

namespace restc_cpp {

std::string url_encode(const boost::string_view& src);

} // namespace

#endif // RESTC_CPP_URL_ENCODE_H_
