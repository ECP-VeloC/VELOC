#include "config.hpp"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

//#define __DEBUG
#include "debug.hpp"

static bool check_dir(const std::string dir_name) {
    DIR *entry = opendir(dir_name.c_str());
    if (entry != NULL) {
	closedir(entry);
	return true;
    }
    if (mkdir(dir_name.c_str(), 0755) != 0)
	return false;
    return true;
}

bool config_t::init(const std::string &cfg_file) {
    std::string val;
    try {
	read_ini(cfg_file, pt);
	if (!check_dir(get("scratch")))
	    throw std::runtime_error("scratch directory " + val + " inaccessible!");
	if (!check_dir(get("persistent")))
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
