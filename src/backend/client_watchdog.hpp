#ifndef __CLIENT_WATCHDOG_HPP
#define __CLIENT_WATCHDOG_HPP

#include "common/config.hpp"
#include "common/command.hpp"

#include <thread>
#include <chrono>
#include <map>

class client_watchdog_t {
    std::thread watchdog_thread;
    const config_t &cfg;
    std::map<int, std::chrono::system_clock::time_point> client_map;
    int timeout;

    void timeout_check();
public:
    client_watchdog_t(const config_t &c);
    int process_command(const command_t &cmd);
};

#endif // __CLIENT_WATCHDOG_HPP
