#include "transfer_module.hpp"

#include "axl.h"
#include "common/file_util.hpp"

#include <unistd.h>

//#define __DEBUG
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
	return posix_transfer_file(source, dest) ? VELOC_SUCCESS : VELOC_FAILURE;
}

int transfer_module_t::process_command(const command_t &c) {
    if (interval < 0)
        return VELOC_IGNORED;

    std::string local = c.filename(cfg.get("scratch")),
	remote = c.filename(cfg.get("persistent"));

    switch (c.command) {
    case command_t::INIT:
	last_timestamp[c.unique_id] = std::chrono::system_clock::now() + std::chrono::seconds(interval);
	return VELOC_SUCCESS;

    case command_t::CHECKPOINT:
	if (interval > 0) {
	    auto t = std::chrono::system_clock::now();
	    if (t < last_timestamp[c.unique_id])
		return VELOC_SUCCESS;
	    else
		last_timestamp[c.unique_id] = t + std::chrono::seconds(interval);
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
        DBG("checking local file: " << local);
        if (access(local.c_str(), R_OK) == 0)
	    return VELOC_SUCCESS;
	if (access(remote.c_str(), R_OK) != 0) {
	    ERROR("request to transfer file " << remote << " to " << local << " failed: source does not exist");
	    return VELOC_IGNORED;
	}
        DBG("transfer file " << remote << " to " << local);
	return transfer_file(remote, local) == VELOC_SUCCESS ? VELOC_SUCCESS : VELOC_IGNORED;

    default:
	return VELOC_IGNORED;
    }
}
