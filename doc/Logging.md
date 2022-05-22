# Logging
 
Logging is often a pain in a library like restc_cpp.

restc_cpp use a C++ streams operator logging scheme internally. It is based on a
few macros, and can be mapped to [logfault](https://github.com/jgaa/logfault), *boost.Log*,
*std::clog* or an internal handler that allow you to register a callback to deal with log events. 

One of the following CMAKE macros can be enabled at compile time:
 
- `RESTC_CPP_LOG_WITH_INTERNAL_LOG`: Use internal log handler and register a callback to handle log events
- `RESTC_CPP_LOG_WITH_LOGFAULT`: Use logfault for logging
- `RESTC_CPP_LOG_WITH_BOOST_LOG`: Use boost::log for logging
- `RESTC_CPP_LOG_WITH_CLOG`: Just send log-events to std::clog

The default (as of version 0.92 of restc_cpp) is `RESTC_CPP_LOG_WITH_INTERNAL_LOG`.

## Example

This example assumes that the CMAKE variable `RESTC_CPP_LOG_WITH_INTERNAL_LOG` is `ON`.

```c++

  restc_cpp::Logger::Instance().SetLogLevel(restc_cpp::LogLevel::DEBUG);

  restc_cpp::Logger::Instance().SetHandler([](restc_cpp::LogLevel level,
                                             const std::string& msg) {
    static const std::array<std::string, 6> levels = {"NONE", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::clog << std::put_time(std::localtime(&now), "%c") << ' '
              << levels.at(static_cast<size_t>(level))
              << ' ' << std::this_thread::get_id() << ' '
              << msg << std::endl;
  });

```

