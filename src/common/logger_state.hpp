#ifndef __LOGGER_STATE
#define __LOGGER_STATE

#include <iostream>
#include <chrono>
#include <mutex>

struct logger_state_t {
    std::ostream *logger = &std::cout;
    std::chrono::time_point<std::chrono::steady_clock> beginning = std::chrono::steady_clock::now();
    std::mutex log_mutex;
};

#endif //__LOGGER_STATE
