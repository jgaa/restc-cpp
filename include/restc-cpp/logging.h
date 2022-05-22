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

#ifdef RESTC_CPP_LOG_WITH_INTERNAL_LOG

#include <iostream>
#include <sstream>
#include <functional>
#include <cassert>
#include <chrono>
#include <thread>
#include <iomanip>

namespace restc_cpp  {

enum class LogLevel {
    MUTED, // Don't log anything at this level
    LERROR,  // Thank you Microsoft, for polluting the global namespace with all your crap
    WARNING,
    INFO,
    DEBUG,
    TRACE
};

class LogEvent {
public:
    LogEvent(LogLevel level)
        : level_{level} {}

    ~LogEvent();

    std::ostringstream& Event() { return msg_; }
    LogLevel Level() const noexcept { return level_; }

private:
    const LogLevel level_;
    std::ostringstream msg_;
};

/*! Internal logger class
 *
 *  This is a verty simple internal log handler that is designd to
 *  forward log events to whatever log framework you use in your
 *  application.
 *
 *  You must set a handler before you call any methods in restc,
 *  and once set, the handler cannot be changed. This is simply
 *  to avoid a memory barrier each time the library create a
 *  log event, just to access the log handler.
 *
 *  You can change the log level at any time, to whatever log
 *  level is enabled at compile time. See CMAKE variable `RESTC_CPP_LOG_LEVEL_STR`.
 */
class Logger {
    public:

    using log_handler_t = std::function<void(LogLevel level, const std::string& msg)>;

    Logger() = default;

    static Logger& Instance() noexcept;

    LogLevel GetLogLevel() const noexcept {return current_; }
    void SetLogLevel(LogLevel level) { current_ = level; }

    /*! Set a log handler.
     *
     *  This can only be done once, and should be done when the library are being
     *  initialized, before any other library methods are called.
     */
    void SetHandler(log_handler_t handler) {
        assert(!handler_);
        handler_ = handler;
    }

    bool Relevant(LogLevel level) const noexcept {
        return handler_ && level <= current_;
    }

    void onEvent(LogLevel level, const std::string& msg) {
        Instance().handler_(level, msg);
    }

private:
    log_handler_t handler_;
    LogLevel current_ = LogLevel::INFO;
};

#define RESTC_CPP_LOG_EVENT(level, msg) Logger::Instance().Relevant(level) && LogEvent{level}.Event() << msg

#define RESTC_CPP_LOG_ERROR_(msg)     RESTC_CPP_LOG_EVENT(LogLevel::LERROR, msg)
#define RESTC_CPP_LOG_WARN_(msg)      RESTC_CPP_LOG_EVENT(LogLevel::WARNING, msg)
#define RESTC_CPP_LOG_INFO_(msg)      RESTC_CPP_LOG_EVENT(LogLevel::INFO, msg)
#define RESTC_CPP_LOG_DEBUG_(msg)     RESTC_CPP_LOG_EVENT(LogLevel::DEBUG, msg)
#define RESTC_CPP_LOG_TRACE_(msg)     RESTC_CPP_LOG_EVENT(LogLevel::TRACE, msg)

}

#define RESTC_CPP_TEST_LOGGING_SETUP(level) RestcCppTestStartLogger(level)

inline void RestcCppTestStartLogger(const std::string& level = "info") {
    auto llevel = restc_cpp::LogLevel::INFO;
    if (level == "debug") {
        llevel = restc_cpp::LogLevel::DEBUG;
    } else if (level == "trace") {
        llevel = restc_cpp::LogLevel::TRACE;
    } else if (level == "info") {
        ;  // Do nothing
    } else {
        std::cerr << "Unknown log-level: " << level << std::endl;
        return;
    }

    restc_cpp::Logger::Instance().SetLogLevel(llevel);

    restc_cpp::Logger::Instance().SetHandler([](restc_cpp::LogLevel level,
                                             const std::string& msg) {
        static const std::array<std::string, 6> levels = {"NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::clog << std::put_time(std::localtime(&now), "%c") << ' '
                  << levels.at(static_cast<size_t>(level))
                  << ' ' << std::this_thread::get_id() << ' '
                  << msg << std::endl;
    });
}

#elif defined RESTC_CPP_LOG_WITH_BOOST_LOG

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

