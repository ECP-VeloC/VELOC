#ifndef __TRANSFER_MODULE_HPP
#define __TRANSFER_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/status.hpp"

#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

class transfer_module_t {
    const config_t &cfg;
    bool use_axl = false;
    std::string axl_type;

    int transfer_file(const std::string &source, const std::string &dest);
public:
    transfer_module_t(const config_t &c);
    ~transfer_module_t();
    int process_command(const command_t &c);
};

#endif //__TRANSFER_MODULE_HPP
