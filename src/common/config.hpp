#ifndef __CONFIG_HPP
#define __CONFIG_HPP

#include "INIReader.h"
#include "storage/storage_module.hpp"
#include <limits>

class config_t {
    std::string cfg_file;
    INIReader reader;
    bool sync_mode = false;
    storage_module_t *sm = NULL;

    static std::string env_param(const std::string &param);

public:
    config_t(const std::string &cfg_file, bool is_backend);
    config_t(const config_t &other) = delete;
    ~config_t();

    std::string get(const std::string &param) const;
    bool get_optional(const std::string &param, std::string &value) const;
    bool get_optional(const std::string &param, int &value) const;
    bool get_optional(const std::string &param, bool def) const;
    bool is_sync() const {
	return sync_mode;
    }
    const std::string &get_cfg_file() const {
	return cfg_file;
    }
    storage_module_t *storage() const {
        return sm;
    }
};

#endif // __CONFIG_HPP
