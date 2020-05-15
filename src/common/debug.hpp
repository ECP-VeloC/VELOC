#ifndef __DEBUG_CONFIG
#define __DEBUG_CONFIG

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <chrono>

static auto beginning = std::chrono::steady_clock::now();

#ifdef __BENCHMARK
#define TIMER_START(timer) auto timer = std::chrono::steady_clock::now();
#define TIMER_STOP(timer, message) {\
        auto now = std::chrono::steady_clock::now();\
	auto d = std::chrono::duration_cast<std::chrono::microseconds>(now - timer).count(); \
        auto t = std::chrono::duration_cast<std::chrono::seconds>(now - beginning).count();\
	std::clog << "[BENCHMARK " << t << "] [" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] [time elapsed: " << d << " us] " << message << std::endl;\
    }
#else
#define TIMER_START(timer)
#define TIMER_STOP(timer, message)
#endif

#define MESSAGE(out, level, message) \
    out << "[" << level << " " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - beginning).count() << "] ["\
        << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] " << message << std::endl

#define FATAL(message) {\
    std::ostringstream out;\
    MESSAGE(out, "FATAL EXCEPTION", message);\
    throw std::runtime_error(out.str());\
}

#ifdef __INFO
#define __ERROR
#define INFO(message) MESSAGE(std::clog, "INFO", message)
#else
#define INFO(message)
#endif

#ifdef __ERROR
#define ERROR(message) MESSAGE(std::clog, "ERROR", message)
#else
#define ERROR(message)
#endif

#ifdef __ASSERT
#define ASSERT(expression) {\
	if (!(expression)) {\
	    std::ostringstream out;\
	    MESSAGE(out, "ASSERT", "failed on expression: " << #expression);\
	    throw std::runtime_error(out.str());\
	}\
    }
#else
#define ASSERT(expression)
#endif

#endif

#undef DBG
#undef DBG_COND
#ifdef __DEBUG
#define DBG(message) MESSAGE(std::clog, "DEBUG", message)
#define DBG_COND(cond, message) if (cond) DBG(message)
#undef __DEBUG
#else
#define DBG(message)
#define DBG_COND(cond, message)
#endif
