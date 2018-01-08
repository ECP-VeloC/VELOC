#ifndef __CONFIG_HPP
#define __CONFIG_HPP

#include <string>

class config_t {
    std::string scratch, persistent;
    bool sync_mode;

public:    
    bool get_parameters(const std::string &cfg_file);
    const std::string &get_scratch() {
	return scratch;
    }
    const std::string &get_persistent() {
	return persistent;
    }
    bool is_sync() {
	return sync_mode;
    }
};

#endif // __CONFIG_HPP
