#include "config.hpp"
#include "file_util.hpp"

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <limits.h>

#include "storage/posix_module.hpp"
#include "storage/axl_module.hpp"

#define __DEBUG
#include "debug.hpp"

std::ostream *logger = &std::cout;

config_t::config_t(const std::string &f, bool is_backend) : cfg_file(f), reader(cfg_file) {
    if (reader.ParseError() < 0)
	FATAL("cannot open config file: " << cfg_file);
    if (reader.ParseError() > 0)
	FATAL("error parsing config file " << cfg_file << " at line " << reader.ParseError());
    std::string val, scratch, persistent;

    // set sync or async mode
    if (get_optional("mode", val)) {
        if (val != "sync" && val != "async")
            FATAL("mode of operation " << val << " is invalid, must be sync/async!");
        sync_mode = (val == "sync");
    }
    // initialize scratch directory
    if (!check_dir(scratch = get("scratch")))
	FATAL("scratch directory " << scratch << " inaccessible!");

    // configure AXL or POSIX
    if (get_optional("persistent", persistent)) {
        if (get_optional("axl_type", val)) {
            INFO("using AXL to interact with persistent storage, AXL type: " << val);
            sm = new axl_module_t(scratch, persistent, val);
        } else {
            INFO("using POSIX to interact with persistent storage, path: " << persistent);
            sm = new posix_module_t(scratch, persistent);
        }
    }

    // configure logging
    std::string log_prefix;
    if (!get_optional("log_prefix", log_prefix)) {
        if (is_backend)
            log_prefix = "/dev/shm";
        else
            return;
    }
    DBG("Log prefix=" << log_prefix);
    char host_name[HOST_NAME_MAX] = "";
    gethostname(host_name, HOST_NAME_MAX);
    std::string log_file = log_prefix + "/" + (is_backend ? "veloc-backend-" : "veloc-client-")
        + std::string(host_name) + "-" + std::to_string(getuid()) + ".log";
    try {
        logger = new std::ofstream(log_file, std::ofstream::out | std::ofstream::trunc);
    } catch(std::exception &e) {
        FATAL("cannot log to " << log_file << ", error: " << e.what());
    }
}

config_t::~config_t() {
    delete sm;
    if (logger != &std::cout) {
        delete logger;
        logger = &std::cout;
    }
}

std::string config_t::get(const std::string &param) const {
    std::string ret = reader.Get("", param, "");
    if (ret.empty())
        FATAL("config parameter " << param << " missing or empty");
    return ret;
}
