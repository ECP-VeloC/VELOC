#ifndef __STORAGE_MODULE_HPP
#define __STORAGE_MODULE_HPP

#include <set>
#include "common/command.hpp"

class storage_module_t {
public:
    virtual void get_versions(const command_t &cmd, std::set<int> &result) = 0;
    virtual bool remove(const command_t &cmd) = 0;
    virtual bool flush(const command_t &cmd) = 0;
    virtual bool restore(const command_t &cmd) = 0;
    virtual bool exists(const command_t &cmd) = 0;
    virtual ~storage_module_t() = 0;
};

#endif //__STORAGE_MODULE_HPP
