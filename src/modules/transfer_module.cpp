#include "transfer_module.hpp"

#include "axl.h"
#include "common/file_util.hpp"

#include <unistd.h>

#define __DEBUG
#include "common/debug.hpp"

transfer_module_t::transfer_module_t(const config_t &c) : cfg(c), axl_type(AXL_XFER_NULL) {
    std::string axl_config, axl_type_str;

    std::map<std::string, axl_xfer_t> axl_type_strs = {
        {"default", AXL_XFER_DEFAULT},
        {"native", AXL_XFER_NATIVE},
        {"AXL_XFER_SYNC", AXL_XFER_SYNC},
        {"AXL_XFER_ASYNC_DW", AXL_XFER_ASYNC_DW},
        {"AXL_XFER_ASYNC_BBAPI", AXL_XFER_ASYNC_BBAPI},
        {"AXL_XFER_ASYNC_CPPR", AXL_XFER_ASYNC_CPPR},
    };

    if (!cfg.get_optional("persistent_interval", interval)) {
	INFO("Persistence interval not specified, every checkpoint will be persisted");
	interval = 0;
    }
    if (!cfg.get_optional("max_versions", max_versions))
	max_versions = 0;

    /* Did the user specify an axl_type in the config file? */
    if (cfg.get_optional("axl_type", axl_type_str)) {
        auto e = axl_type_strs.find(axl_type_str);
        if (e == axl_type_strs.end()) {
            axl_type = AXL_XFER_NULL;
        } else {
            axl_type = e->second;
        }

        if (axl_type == AXL_XFER_NULL) {
            /* It's an invalid axl_type */
            ERROR("AXL has no transfer type called \"" << axl_type_str <<"\"");
            ERROR("Valid transfer types are:");
            for (auto s = axl_type_strs.cbegin(); s != axl_type_strs.cend(); s++) {
                ERROR("\t" << s->first);
            }
            return;
        } else {
            axl_type = e->second;
        }

    } else {
        INFO("AXL transfer type (axl_type) missing or invalid, deactivated!");
        return;
    }

    int ret = AXL_Init(NULL);
    if (ret)
        ERROR("AXL initialization failure, error code: " << ret << "; falling back to POSIX");
    else {
        INFO("AXL successfully initialized");
        use_axl = true;
    }
}

transfer_module_t::~transfer_module_t() {
    AXL_Finalize();
}

static int axl_transfer_file(axl_xfer_t type, const std::string &source, const std::string &dest) {
    int id = AXL_Create(type, source.c_str());
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
	return axl_transfer_file(axl_type, source, dest);
    else
	return posix_transfer_file(source, dest);
}

int transfer_module_t::process_command(const command_t &c) {
    std::string local = c.filename(cfg.get("scratch")),
	remote = c.filename(cfg.get("persistent"));

   switch (c.command) {
    case command_t::INIT:
	if (interval < 0)
	    return VELOC_SUCCESS;
	last_timestamp[c.unique_id] = std::chrono::system_clock::now() + std::chrono::seconds(interval);
	return VELOC_SUCCESS;
	
    case command_t::TEST:
	DBG("rank " << c.unique_id << ": obtain latest version for " << c.name);
	return std::max(get_latest_version(cfg.get("scratch"), std::string(c.name) + "-" + std::to_string(c.unique_id), c.version),
			get_latest_version(cfg.get("persistent"), std::string(c.name) + "-" + std::to_string(c.unique_id), c.version));
	
    case command_t::CHECKPOINT:
	if (interval < 0) 
	    return VELOC_SUCCESS;
	if (interval > 0) {
	    auto t = std::chrono::system_clock::now();
	    if (t < last_timestamp[c.unique_id])
		return VELOC_SUCCESS;
	    else
		last_timestamp[c.unique_id] = t + std::chrono::seconds(interval);
	}
	// remove old versions if needed
	if (max_versions > 0) {
	    auto &version_history = checkpoint_history[c.unique_id][c.name];
	    version_history.push_back(c.version);
	    if ((int)version_history.size() > max_versions) {
		unlink(c.filename(cfg.get("persistent"), version_history.front()).c_str());
		version_history.pop_front();
	    }
	}
	DBG("transfer file " << local << " to " << remote);
	if (c.original[0] == 0)
	    return transfer_file(local, remote);
	else {
	    // at this point, we in file-based mode with custom file names
	    if (transfer_file(local, c.original) == VELOC_FAILURE)
		return VELOC_FAILURE;
	    unlink(remote.c_str());
	    if (symlink(c.original, remote.c_str()) != 0) {
		ERROR("cannot create symlink " << remote.c_str() << " pointing at " << c.original << ", error: " << std::strerror(errno));
		return VELOC_FAILURE;
	    } else
		return VELOC_SUCCESS;	
	}
	
    case command_t::RESTART:
	if (access(local.c_str(), R_OK) == 0)
	    return VELOC_SUCCESS;

        DBG("transfer file " << remote << " to " << local);
	if (access(remote.c_str(), R_OK) != 0) {
	    ERROR("request to transfer file " << remote << " to " << local << " failed: source does not exist");
	    return VELOC_FAILURE;
	}
	if (max_versions > 0) {
	    auto &version_history = checkpoint_history[c.unique_id][c.name];
	    version_history.clear();
	    version_history.push_back(c.version);
	}
	return transfer_file(remote, local);
	
    default:
	return VELOC_SUCCESS;
    }
}
