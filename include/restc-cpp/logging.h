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

#define RESTC_CPP_LOG_ERROR_(msg)     BOOST_LOG_TRIVIAL(error) << msg
#define RESTC_CPP_LOG_WARN_(msg)      BOOST_LOG_TRIVIAL(warning) << msg
#define RESTC_CPP_LOG_INFO_(msg)      BOOST_LOG_TRIVIAL(info) << msg
#define RESTC_CPP_LOG_DEBUG_(msg)     BOOST_LOG_TRIVIAL(debug) << msg
#define RESTC_CPP_LOG_TRACE_(msg)     BOOST_LOG_TRIVIAL(trace) << msg


#else
// The user of the API framework may provide log macros, or we disable logs

#   ifdef RESTC_CPP_LOG_ERROR
#       define RESTC_CPP_LOG_ERROR_(msg)     RESTC_CPP_LOG_ERROR << msg
#   else
#       define RESTC_CPP_LOG_ERROR_(msg)
#   endif

#   ifdef RESTC_CPP_LOG_WARN
#       define RESTC_CPP_LOG_WARN_(msg)     RESTC_CPP_LOG_WARN << msg
#   else
#       define RESTC_CPP_LOG_WARN_(msg)
#   endif


#   ifdef RESTC_CPP_LOG_DEBUG
#       define RESTC_CPP_LOG_DEBUG_(msg)     RESTC_CPP_LOG_DEBUG << msg
#   else
#       define RESTC_CPP_LOG_DEBUG_(msg)
#   endif


#   ifdef RESTC_CPP_LOG_INFO
#       define RESTC_CPP_LOG_INFO_(msg)     RESTC_CPP_LOG_INFO << msg
#   else
#       define RESTC_CPP_LOG_INFO_(msg)
#   endif


#   ifdef RESTC_CPP_LOG_TRACE
#       define RESTC_CPP_LOG_TRACE_(msg)     RESTC_CPP_LOG_TRACE << msg
#   else
#       define RESTC_CPP_LOG_TRACE_(msg)
#   endif

#endif
