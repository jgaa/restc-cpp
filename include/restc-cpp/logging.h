#pragma once

/*
 * We define our own macros for logging and map them to whatever
 * log framework the user prefers. The only requirement is that
 * the log framework support stream like logging.
 *
 * As a starter, we will support boost::log.
 *
 */

#include "restc-cpp/config.h"

#ifdef RESTC_CPP_LOG_WITH_BOOST_LOG

#ifndef WIN32
#	define BOOST_LOG_DYN_LINK 1
#endif

#include <boost/log/trivial.hpp>

#define RESTC_CPP_LOG_ERROR     BOOST_LOG_TRIVIAL(error)
#define RESTC_CPP_LOG_WARN      BOOST_LOG_TRIVIAL(warning)
#define RESTC_CPP_LOG_INFO      BOOST_LOG_TRIVIAL(info)
#define RESTC_CPP_LOG_DEBUG     BOOST_LOG_TRIVIAL(debug)
#define RESTC_CPP_LOG_TRACE     BOOST_LOG_TRIVIAL(trace)


#else
// The user of the API framework must provide log macros
#error "No log framework is selected"
#endif
