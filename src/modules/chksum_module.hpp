#ifndef __CHKSUM_MODULE_HPP
#define __CHKSUM_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/status.hpp"

class chksum_module_t {
    bool active;
    const config_t &cfg;
public:
    chksum_module_t(const config_t &c);
    ~chksum_module_t() { }
    int process_command(const command_t &c);
};

#endif //__CHKSUM_MODULE_HPP
