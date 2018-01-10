#include <boost/filesystem.hpp>

#include "config.hpp"

#include "debug.hpp"

namespace bf = boost::filesystem;

bool config_t::init(const std::string &cfg_file) {
    std::string val;
    try {
	read_ini(cfg_file, pt);
	if (!bf::is_directory(val = get("scratch")))
	    throw std::runtime_error("scratch directory " + val + " is invalid!");
	if (!bf::is_directory(val = get("persistent")))
	    throw std::runtime_error("persistent directory " + val + " is invalid!");
	if (!bf::exists(val = get("axl_config")))
	    throw std::runtime_error("AXL configuration file " + val + " is invalid!");
	val = get("axl_type");
	if (val != "posix" && val != "bb" && val != "dw")
	    throw std::runtime_error("AXL transfer type " + val + " is invalid!");
	val = get("mode");
	if (val != "sync" && val != "async")
	    throw std::runtime_error("mode of operation " + val + " is invalid!");
	sync_mode = (val == "sync");
    } catch (std::exception &e) {
	ERROR("Invalid configuration file " << cfg_file << ", error is: " << e.what());
	return false;
    }
    return true;
}
