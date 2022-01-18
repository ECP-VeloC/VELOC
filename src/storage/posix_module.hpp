#ifndef __POSIX_MODULE_HPP
#define __POSIX_MODULE_HPP

#include "storage_module.hpp"

class posix_module_t : public storage_module_t {
protected:
    std::string scratch, persistent;

public:
    posix_module_t(const std::string &scratch, const std::string &persistent);
    virtual ~posix_module_t();
    virtual void get_versions(const command_t &cmd, std::set<int> &result);
    virtual bool remove(const command_t &cmd);
    virtual bool flush(const command_t &cmd);
    virtual bool restore(const command_t &cmd);
    virtual bool exists(const command_t &cmd);
};

#endif //__POSIX_MODULE_HPP
