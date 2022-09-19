#ifndef __TRANSFER_MODULE_HPP
#define __TRANSFER_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/status.hpp"

#include <chrono>
#include <map>
#include <mutex>

class transfer_module_t {
    const config_t &cfg;
    int interval;
    std::mutex ts_lock;
    std::map<int, std::chrono::system_clock::time_point> last_timestamp;

    int transfer_file(const std::string &source, const std::string &dest);
public:
    transfer_module_t(const config_t &c);
    int process_command(const command_t &c);
};

#endif //__TRANSFER_MODULE_HPP
