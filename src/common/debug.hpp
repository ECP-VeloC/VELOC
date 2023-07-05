#ifndef __DEBUG_CONFIG
#define __DEBUG_CONFIG

#include <cstring>
#include "logger_state.hpp"

extern logger_state_t logger_state;

#ifdef __BENCHMARK
#define TIMER_START(timer) auto timer = std::chrono::steady_clock::now();
#define TIMER_STOP(timer, message) {\
        auto now = std::chrono::steady_clock::now();\
	auto d = std::chrono::duration_cast<std::chrono::milliseconds>(now - timer).count();\
        auto t = std::chrono::duration_cast<std::chrono::seconds>(now - logger_state.beginning).count();\
        std::unique_lock<std::mutex> lock(logger_state.log_mutex);\
	if (logger_state.logger != nullptr)\
	    (*logger_state.logger) << "[BENCHMARK " << t << "] [" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__\
				   << "] [time elapsed: " << d << " us] " << message << std::endl; \
    }
#else
#define TIMER_START(timer)
#define TIMER_STOP(timer, message)
#endif

#define MESSAGE(level, message) {\
    std::unique_lock<std::mutex> lock(logger_state.log_mutex);\
    auto now = std::chrono::steady_clock::now();\
    if (logger_state.logger != nullptr)\
        (*logger_state.logger) << "[" << level << " " << std::chrono::duration_cast<std::chrono::seconds>(now - logger_state.beginning).count()\
			       << "] [" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] " << message << std::endl; \
}

#define FATAL(message) {\
    MESSAGE("FATAL", message);\
    exit(-1);\
}

#ifdef __INFO
#define __ERROR
#define INFO(message) MESSAGE("INFO", message)
#else
#define INFO(message)
#endif

#ifdef __ERROR
#define ERROR(message) MESSAGE("ERROR", message)
#else
#define ERROR(message)
#endif

#ifdef __ASSERT
#define ASSERT(expression) {\
	if (!(expression)) {\
	    MESSAGE("ASSERT", "failed on expression: " << #expression);\
	    exit(-2);\
	}\
    }
#else
#define ASSERT(expression)
#endif

#endif

#undef DBG
#undef DBG_COND
#ifdef __DEBUG
#define DBG(message) MESSAGE("DEBUG", message)
#define DBG_COND(cond, message) if (cond) DBG(message)
#undef __DEBUG
#else
#define DBG(message)
#define DBG_COND(cond, message)
#endif
