#ifndef __CONFIG_HPP
#define __CONFIG_HPP

#include "INIReader.h"

#include <limits>

class config_t {
    bool sync_mode;
    INIReader reader;
public:    
    config_t(const std::string &cfg_file);
    std::string get(const std::string &param) const {
	std::string ret = reader.Get("", param, "");
	if (ret.empty())
	    throw std::runtime_error("config parameter " + param + " missing or empty");
	return ret;

    }
    bool get_optional(const std::string &param, std::string &value) const {
	value = reader.Get("", param, "");
	return !value.empty();
    }
    bool get_optional(const std::string &param, int &value) const {
	value = reader.GetInteger("", param, std::numeric_limits<int>::lowest());
	return value != std::numeric_limits<int>::lowest();
    }
    bool is_sync() {
	return sync_mode;
    }
};

#endif // __CONFIG_HPP
