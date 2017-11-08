#ifndef __DEBUG_CONFIG
#define __DEBUG_CONFIG

#include <stdexcept>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>

#define safe_printf(...) {\
    char msg[1024];\
    sprintf(msg, __VA_ARGS__);\
    std::cout << msg;}

#ifdef __BENCHMARK
#define TIMER_START(timer) boost::posix_time::ptime timer(boost::posix_time::microsec_clock::local_time());
#define TIMER_STOP(timer, message) {\
	boost::posix_time::ptime now(boost::posix_time::microsec_clock::local_time());\
	boost::posix_time::time_duration t = now - timer;\
	std::clog << "[BENCHMARK " << now << "] [" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "] [time elapsed: " << t << " us] " << message << std::endl;\
    }
#else
#define TIMER_START(timer)
#define TIMER_STOP(timer, message)
#endif

#define MESSAGE(out, level, message)\
    out << "[" << level << " " << boost::posix_time::microsec_clock::local_time() << "] [" \
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
