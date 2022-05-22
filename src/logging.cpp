

#include "restc-cpp/logging.h"

using namespace std;

namespace restc_cpp {

Logger &Logger::Instance() noexcept
{
    static Logger logger;
    return logger;
}

LogEvent::~LogEvent()
{
    Logger::Instance().onEvent(level_, msg_.str());
}

}
