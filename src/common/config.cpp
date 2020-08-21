#include "config.hpp"
#include "file_util.hpp"

//#define __DEBUG
#include "debug.hpp"

config_t::config_t(const std::string &f) : cfg_file(f), reader(cfg_file) {
    if (reader.ParseError() < 0)
	FATAL("cannot open config file: " << cfg_file);
    if (reader.ParseError() > 0)
	FATAL("error parsing config file " << cfg_file << " at line " << reader.ParseError());
    std::string val;
    if (!check_dir(val = get("scratch")))
	FATAL("scratch directory " << val << " inaccessible!");
    if (!check_dir(val = get("persistent")))
	FATAL("persistent directory " << val << " inaccessible!");
    val = get("mode");
    if (val != "sync" && val != "async")
	FATAL("mode of operation " << val << " is invalid, must be sync/async!");
    sync_mode = (val == "sync");
}

std::string config_t::get(const std::string &param) const {
    std::string ret = reader.Get("", param, "");
    if (ret.empty())
        FATAL("config parameter " << param << " missing or empty");
    return ret;
}
