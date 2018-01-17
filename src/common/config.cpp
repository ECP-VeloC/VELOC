#include <boost/filesystem.hpp>

#include "config.hpp"

#include "debug.hpp"

namespace bf = boost::filesystem;

bool config_t::init(const std::string &cfg_file) {
    std::string val;
    try {
	read_ini(cfg_file, pt);
	if (!bf::is_directory(val = get("scratch")) && !bf::create_directory(val))
	    throw std::runtime_error("scratch directory " + val + " inaccessible!");
	if (!bf::is_directory(val = get("persistent")) && !bf::create_directory(val))
	    throw std::runtime_error("persistent directory " + val + " inaccessible!");
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
