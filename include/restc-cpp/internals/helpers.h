#pragma once

#include <mutex>

#ifdef RESTC_CPP_THREADED_CTX
#   define LOCK_ std::lock_guard<std::mutex> lock_{mutex_}
#else
#   define LOCK_
#endif
