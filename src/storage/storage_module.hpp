#ifndef __STORAGE_MODULE_HPP
#define __STORAGE_MODULE_HPP

#include <set>
#include "common/command.hpp"

class storage_module_t {
public:
    storage_module_t(...);
    virtual void get_versions(const command_t &cmd, std::set<int> &result);
    virtual bool remove(const command_t &cmd);
    virtual bool flush(const command_t &cmd);
    virtual bool restore(const command_t &cmd);
    virtual bool exists(const command_t &cmd);
    virtual ~storage_module_t();
};

#endif //__STORAGE_MODULE_HPP
