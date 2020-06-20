#include "config.hpp"
#include "file_util.hpp"

//#define __DEBUG
#include "debug.hpp"

config_t::config_t(const std::string &f) : cfg_file(f), reader(cfg_file) {
    if (reader.ParseError() < 0)
	throw std::runtime_error("cannot open config file " + cfg_file);
    if (reader.ParseError() > 0)
	throw std::runtime_error("error parsing config file " + cfg_file + " at line " + std::to_string(reader.ParseError()));
    std::string val;
    if (!check_dir(val = get("scratch")))
	throw std::runtime_error("scratch directory " + val + " inaccessible!");
    if (!check_dir(val = get("persistent")))
	throw std::runtime_error("persistent directory " + val + " inaccessible!");
    val = get("mode");
    if (val != "sync" && val != "async")
	throw std::runtime_error("mode of operation " + val + " is invalid, must be sync/async!");
    sync_mode = (val == "sync");
}
