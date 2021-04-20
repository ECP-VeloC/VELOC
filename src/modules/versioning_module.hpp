#ifndef __VERSIONING_MODULE_HPP
#define __VERSIONING_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"

#include <mutex>
#include <set>
#include <map>

class versioning_module_t {
    std::mutex history_lock;
    typedef std::map<int, std::set<int> > checkpoint_history_t;
    std::map<std::string, checkpoint_history_t> persistent_history, scratch_history;
    int max_versions, scratch_versions;
    const config_t &cfg;

public:
    versioning_module_t(const config_t &c);
    ~versioning_module_t() { }
    int process_command(const command_t &c);
};

#endif
