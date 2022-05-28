#ifndef __POSIX_AGG_MODULE_HPP
#define __POSIX_AGG_MODULE_HPP

#include "posix_module.hpp"

class posix_agg_module_t : public posix_module_t {
protected:
    std::string meta;

public:
    posix_agg_module_t(const std::string &scratch, const std::string &persistent, const std::string &meta);
    virtual ~posix_agg_module_t();
    virtual void get_versions(const command_t &cmd, std::set<int> &result);
    virtual bool remove(const command_t &cmd);
    virtual bool flush(const command_t &cmd);
    virtual bool restore(const command_t &cmd);
    virtual bool exists(const command_t &cmd);
};

#endif //__POSIX_AGG_MODULE_HPP
