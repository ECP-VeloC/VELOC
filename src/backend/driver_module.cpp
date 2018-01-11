#include "driver_module.hpp"
// #include "axl.h"

#include <boost/range.hpp>

#define __DEBUG
#include "common/debug.hpp"

driver_module_t::driver_module_t(const config_t &c) : cfg(c) {
    // int ret = AXL_Init(cfg.get("axl_config").c_str());
    // if (ret)
    // 	throw std::runtime_error("AXL initialization failed, error code is: " + std::to_string(ret));
}

driver_module_t::~driver_module_t() {
    // AXL_Finalize();
}

int driver_module_t::get_latest_version(const bf::path &p, const std::string &cname, int needed_id) {
    int ret = -1;
    for(auto& f : boost::make_iterator_range(bf::directory_iterator(p), {})) {
	std::string fname = f.path().leaf().string();
	int id, version;
	if (bf::is_regular_file(f.path()) &&
	    fname.compare(0, cname.length(), cname) == 0 &&
	    sscanf(fname.substr(cname.length()).c_str(), "-%d-%d", &id, &version) == 2 &&
	    id == needed_id) {	 
	    if (version > ret)
		    ret = version;
	}
    }
    return ret;
}

int driver_module_t::transfer_file(const std::string &source, const std::string &dest) {
    // int id = AXL_Create(cfg.get("axl_type"), bf::path(c.ckpt_name).filename());
    // if (id < 0)
    // 	return VELOC_FAILURE;
    // if (AXL_Add(id, source.c_str(), dest.c_str()))
    // 	return VELOC_FAILURE;
    // if (AXL_Dispatch(id))
    // 	return VELOC_FAILURE;
    // if (AXL_Wait(id))
    // 	return VELOC_FAILURE;
    // if (AXL_Free(id))
    // 	return VELOC_FAILURE;
    return VELOC_SUCCESS;
}

int driver_module_t::process_command(const command_t &c) {
    std::string remote = (bf::path(cfg.get("persistent")) / bf::path(c.ckpt_name).filename()).string();
    switch (c.command) {
    case command_t::TEST:
	DBG("obtain latest version for " << c.ckpt_name);
	return std::max(get_latest_version(cfg.get("scratch"), c.ckpt_name, c.unique_id),
			get_latest_version(cfg.get("persistent"), c.ckpt_name, c.unique_id));
	
    case command_t::CHECKPOINT:
	DBG("transfer file " << c.ckpt_name << " to " << remote);
	return transfer_file(c.ckpt_name, remote);

    case command_t::RESTART:
	DBG("transfer file " << remote << " to " << c.ckpt_name);
	if (bf::exists(c.ckpt_name)) {
	    INFO("request to transfer file " << remote << " to " << c.ckpt_name << " as destination already exists");
	    return VELOC_SUCCESS;
	}
	if (!bf::exists(remote)) {
	    ERROR("request to transfer file " << remote << " to " << c.ckpt_name << " ignored as source does not exist");
	    return VELOC_FAILURE;
	}
	return transfer_file(c.ckpt_name, remote);
	
    default:
	return VELOC_SUCCESS;
    }
}
