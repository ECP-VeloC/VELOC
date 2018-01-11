#ifndef __CONFIG_HPP
#define __CONFIG_HPP

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/optional.hpp>

class config_t {
    boost::property_tree::ptree pt;
    bool sync_mode;
    
public:    
    bool init(const std::string &cfg_file);
    const std::string get(const std::string &param) const {
	return pt.get<std::string>(param);
    }
    template <class T> const boost::optional<T> get_optional(const std::string &param) const {
	return pt.get_optional<T>(param);
    }
    bool is_sync() {
	return sync_mode;
    }
};

#endif // __CONFIG_HPP
