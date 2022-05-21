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

#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#define RESTC_CPP_LOG_ERROR_(msg)     BOOST_LOG_TRIVIAL(error) << msg
#define RESTC_CPP_LOG_WARN_(msg)      BOOST_LOG_TRIVIAL(warning) << msg
#define RESTC_CPP_LOG_INFO_(msg)      BOOST_LOG_TRIVIAL(info) << msg
#define RESTC_CPP_LOG_DEBUG_(msg)     BOOST_LOG_TRIVIAL(debug) << msg
#define RESTC_CPP_LOG_TRACE_(msg)     BOOST_LOG_TRIVIAL(trace) << msg

#define RESTC_CPP_TEST_LOGGING_SETUP(level) RestcCppTestStartLogger(level)

inline void RestcCppTestStartLogger(const std::string& level = "info") {
    auto llevel = boost::log::trivial::info;
    if (level == "debug") {
        llevel = boost::log::trivial::debug;
    } else if (level == "trace") {
        llevel = boost::log::trivial::trace;
    } else if (level == "info") {
        ;  // Do nothing
    } else {
        std::cerr << "Unknown log-level: " << level << std::endl;
        return;
    }

    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= llevel
    );
}

#elif defined RESTC_CPP_LOG_WITH_LOGFAULT

#include <iostream>
#include <memory>
#include "logfault/logfault.h"

#define RESTC_CPP_LOG_ERROR_(msg)     LFLOG_ERROR << msg
#define RESTC_CPP_LOG_WARN_(msg)      LFLOG_WARN << msg
#define RESTC_CPP_LOG_INFO_(msg)      LFLOG_INFO << msg
#define RESTC_CPP_LOG_DEBUG_(msg)     LFLOG_DEBUG << msg
#define RESTC_CPP_LOG_TRACE_(msg)     LFLOG_TRACE << msg

#define RESTC_CPP_TEST_LOGGING_INCLUDE
#define RESTC_CPP_TEST_LOGGING_SETUP(level) RestcCppTestStartLogger(level)

inline void RestcCppTestStartLogger(const std::string& level = "info") {
    auto llevel = logfault::LogLevel::INFO;
    if (level == "debug") {
        llevel = logfault::LogLevel::DEBUGGING;
    } else if (level == "trace") {
        llevel = logfault::LogLevel::TRACE;
    } else if (level == "info") {
        ;  // Do nothing
    } else {
        std::cerr << "Unknown log-level: " << level << std::endl;
        return;
    }

    logfault::LogManager::Instance().AddHandler(
                std::make_unique<logfault::StreamHandler>(std::clog, llevel));
}


#elif defined RESTC_CPP_LOG_WITH_CLOG

#include <iostream>

#define RESTC_CPP_TEST_LOGGING_SETUP(level)

#define RESTC_CPP_LOG_ERROR_(msg)     std::clog << "ERROR " << msg << std::endl
#define RESTC_CPP_LOG_WARN_(msg)      std::clog << "WARN  " << msg << std::endl
#define RESTC_CPP_LOG_INFO_(msg)      std::clog << "INFO  " << msg << std::endl
#define RESTC_CPP_LOG_DEBUG_(msg)     std::clog << "DEBUG " << msg << std::endl
#define RESTC_CPP_LOG_TRACE_(msg)     std::clog << "TRACE " << msg << std::endl


#else
// The user of the API framework may provide log macros, or we disable logs

#define RESTC_CPP_TEST_LOGGING_SETUP(level)

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

// Limit the log-level to RESTC_CPP_LOG_LEVEL
// No need to spam production logs.
// By using macros, we also eliminate CPU cycles on irrelevant log statements

#if (RESTC_CPP_LOG_LEVEL < 5)
#   undef RESTC_CPP_LOG_TRACE_
#   define RESTC_CPP_LOG_TRACE_(msg)
#endif

#if (RESTC_CPP_LOG_LEVEL < 4)
#   undef RESTC_CPP_LOG_DEBUG_
#   define RESTC_CPP_LOG_DEBUG_(msg)
#endif

#if (RESTC_CPP_LOG_LEVEL < 3)
#   undef RESTC_CPP_LOG_INFO_
#   define RESTC_CPP_LOG_TRACE_(msg)
#endif

#if (RESTC_CPP_LOG_LEVEL < 2)
#   undef RESTC_CPP_LOG_WARN_
#   define RESTC_CPP_LOG_WARN_(msg)
#endif

#if (RESTC_CPP_LOG_LEVEL < 1)
#   undef RESTC_CPP_LOG_ERROR_
#   define RESTC_CPP_LOG_ERROR_(msg)
#endif

