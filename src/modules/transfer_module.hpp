#ifndef __TRANSFER_MODULE_HPP
#define __TRANSFER_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/status.hpp"

#include <chrono>
#include <deque>
#include <map>

class transfer_module_t {
    const config_t &cfg;
    int interval, max_versions;
    std::map<int, std::chrono::system_clock::time_point> last_timestamp;
    typedef std::map<std::string, std::deque<int> > checkpoint_history_t;
    std::map<int, checkpoint_history_t> checkpoint_history;

    int transfer_file(const std::string &source, const std::string &dest);
public:
    transfer_module_t(const config_t &c);
    ~transfer_module_t();
    int process_command(const command_t &c);
};

#endif //__TRANSFER_MODULE_HPP
