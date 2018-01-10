#ifndef __DRIVER_MODULE_HPP
#define __DRIVER_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/status.hpp"

#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

class driver_module_t {
    const config_t &cfg;

    int get_latest_version(const bf::path &p, const std::string &cname, int needed_id);
    int transfer_file(const std::string &source, const std::string &dest);
public:
    driver_module_t(const config_t &c);
    ~driver_module_t();
    int process_command(const command_t &c);
};

#endif //__DRIVER_MODULE_HPP
