#ifndef __TRANSFER_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/status.hpp"

#include <chrono>
#include <daos.h>

class daos_module_t {
    const config_t &cfg;
    int interval;
    std::map<int, std::chrono::system_clock::time_point> last_timestamp;
    daos_handle_t poh, coh;

    bool daos_transfer_file(const command_t &c);
public:
    daos_module_t(const config_t &c);
    ~daos_module_t();
    int process_command(const command_t &c);
};

#endif //__TRANSFER_MODULE_HPP
