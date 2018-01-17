#include "transfer_module.hpp"

#ifdef AXL_FOUND
extern "C" {
#include "axl.h"
}
#endif

#include <boost/range.hpp>

#define __DEBUG
#include "common/debug.hpp"

static int posix_transfer_file(const std::string &source, const std::string &dest) {
    DBG("transfer file " << source << " to " << dest);
    try {
	bf::copy_file(source, dest, bf::copy_option::overwrite_if_exists);
    } catch (bf::filesystem_error &e) {
	ERROR("cannot copy " <<  source << " to " << dest << "; error message: " << e.what());
	return VELOC_FAILURE;
    }
    return VELOC_SUCCESS;
}

#ifdef AXL_FOUND // compiled with AXL
transfer_module_t::transfer_module_t(const config_t &c) : cfg(c) {
    auto axl_config = cfg.get_optional<std::string>("axl_config");
    if (!axl_config || !bf::exists(*axl_config)) {
	ERROR("AXL configuration file (axl_config) missing or invalid, deactivated!");
	return;
    }
    auto axl_str = cfg.get_optional<std::string>("axl_type");
    if (!axl_str || (*axl_str != "posix" && *axl_str != "bb" && *axl_str != "dw")) {
	ERROR("AXL transfer type (axl_type) missing or invalid, deactivated!");
	return;
    }
    int ret = AXL_Init((char *)axl_config->c_str());
    if (ret)
	ERROR("AXL initialization failure, error code: " << ret << "; falling back to POSIX");
    else {
	INFO("AXL successfully initialized");
	use_axl = true;
	axl_type = *axl_str;
    }
}

transfer_module_t::~transfer_module_t() {
    AXL_Finalize();
}

static int axl_transfer_file(const char *type, const std::string &source, const std::string &dest) {
    int id = AXL_Create((char *)type, bf::path(source).filename().c_str());
    if (id < 0)
    	return VELOC_FAILURE;
    if (AXL_Add(id, (char *)source.c_str(), (char *)dest.c_str()))
    	return VELOC_FAILURE;
    if (AXL_Dispatch(id))
    	return VELOC_FAILURE;
    if (AXL_Wait(id))
    	return VELOC_FAILURE;
    if (AXL_Free(id))
    	return VELOC_FAILURE;
    return VELOC_SUCCESS;
}

int transfer_module_t::transfer_file(const std::string &source, const std::string &dest) {
    if (use_axl)
	return axl_transfer_file(axl_type.c_str(), source, dest);
    else
	return posix_transfer_file(source, dest);
}

#else // compiled without AXL
transfer_module_t::transfer_module_t(const config_t &c) : cfg(c) { }
transfer_module_t::~transfer_module_t() { }

int transfer_module_t::transfer_file(const std::string &source, const std::string &dest) {
    return posix_transfer_file(source, dest);
}
#endif

static int get_latest_version(const bf::path &p, const std::string &cname, int needed_id) {
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

int transfer_module_t::process_command(const command_t &c) {
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
	return transfer_file(remote, c.ckpt_name);
	
    default:
	return VELOC_SUCCESS;
    }
}
