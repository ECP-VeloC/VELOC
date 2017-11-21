#ifndef __CONFIG_HPP
#define __CONFIG_HPP

class config_t {
    std::string scratch, persistent;
    
public:    
    bool get_parameters(const std::string &cfg_file);
    const std::string &get_scratch() {
	return scratch;
    }
    const std::string &get_persistent() {
	return persistent;
    }
};

#endif
