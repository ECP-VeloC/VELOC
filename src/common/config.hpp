#ifndef __CONFIG_HPP
#define __CONFIG_HPP

#include "INIReader.h"
#include <limits>

class config_t {
    std::string cfg_file;
    INIReader reader;
    bool sync_mode;
public:
    config_t(const std::string &cfg_file, bool is_backend);
    config_t(const config_t &other) = delete;
    ~config_t();

    std::string get(const std::string &param) const;
    bool get_optional(const std::string &param, std::string &value) const {
	value = reader.Get("", param, "");
	return !value.empty();
    }
    bool get_optional(const std::string &param, int &value) const {
	value = reader.GetInteger("", param, std::numeric_limits<int>::lowest());
	return value != std::numeric_limits<int>::lowest();
    }
    bool get_optional(const std::string &param, bool def) const {
	return reader.GetBoolean("", param, def);
    }
    bool is_sync() const {
	return sync_mode;
    }
    const std::string &get_cfg_file() const {
	return cfg_file;
    }
};

#endif // __CONFIG_HPP
