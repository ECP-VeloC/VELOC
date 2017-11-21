#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>

#include "config.hpp"

#include "debug.hpp"

bool config_t::get_parameters(const std::string &cfg_file) {
    using boost::property_tree::ptree;
    ptree pt;

    try {
	read_ini(cfg_file, pt);
    } catch (std::exception &e) {
	ERROR("Invalid configuration file " << cfg_file << ", error is: " << e.what());
	return false;
    }
    
    if (!boost::filesystem::is_directory(scratch = pt.get("scratch", "/tmp"))) {
	ERROR("Scratch directory " << scratch << " specificied in config file " << cfg_file << " is invalid!");
	return false;
    }
    if (!boost::filesystem::is_directory(persistent = pt.get("persistent", "/tmp"))) {
	ERROR("Persistent directory " << persistent << " specificied in config file " << cfg_file << " is invalid!");
	return false;
    }
    return true;
}
