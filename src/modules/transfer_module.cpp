#include "transfer_module.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>

extern "C" {
#include "axl.h"
}

#define __DEBUG
#include "common/debug.hpp"

static int posix_transfer_file(const std::string &source, const std::string &dest) {
    DBG("transfer file " << source << " to " << dest);
    int fi = open(source.c_str(), O_RDONLY);
    if (fi == -1) {
	ERROR("cannot open source " << source << "; error = " << std::strerror(errno));
	return VELOC_FAILURE;
    }
    int fo = open(dest.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fo == -1) {
	close(fi);
	ERROR("cannot open destination " << dest << "; error = " << std::strerror(errno));
	return VELOC_FAILURE;
    }
    struct stat st;
    stat(source.c_str(), &st);
    size_t remaining = st.st_size;
    while (remaining > 0) {
	ssize_t transferred = sendfile(fo, fi, NULL, remaining);
	if (transferred == -1) {
	    close(fi);
	    close(fo);
	    ERROR("cannot copy " <<  source << " to " << dest << "; error = " << std::strerror(errno));
	    return VELOC_FAILURE;
	} else
	    remaining -= transferred;
    }
    close(fi);
    close(fo);
    return VELOC_SUCCESS;
}

transfer_module_t::transfer_module_t(const config_t &c) : cfg(c), axl_type(AXL_XFER_NULL) {
    std::string axl_config, axl_type_str;

    if (!cfg.get_optional("persistent_interval", interval)) {
	INFO("Persistence interval not specified, every checkpoint will be persisted");
	interval = 0;
    }
    if (!cfg.get_optional("axl_config", axl_config) || access(axl_config.c_str(), R_OK) != 0) {
	ERROR("AXL configuration file (axl_config) missing or invalid, deactivated!");
	return;
    }
    if (!cfg.get_optional("axl_type", axl_type_str) || (axl_type_str != "AXL_XFER_SYNC")) {
	ERROR("AXL transfer type (axl_type) missing or invalid, deactivated!");
	return;
    }    
    int ret = AXL_Init((char *)axl_config.c_str());
    if (ret)
	ERROR("AXL initialization failure, error code: " << ret << "; falling back to POSIX");
    else {
	INFO("AXL successfully initialized");
	use_axl = true;
	axl_type = AXL_XFER_SYNC;
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

static int get_latest_version(const std::string &p, const std::string &cname, int needed_id, int needed_version) {
    struct dirent *dentry;
    DIR *dir;
    int id, version, ret = -1;
    
    dir = opendir(p.c_str());
    if (dir == NULL)
	return -1;
    while ((dentry = readdir(dir)) != NULL) {
	std::string fname = std::string(dentry->d_name);
	if (fname.compare(0, cname.length(), cname) == 0 &&
	    sscanf(fname.substr(cname.length()).c_str(), "-%d-%d", &id, &version) == 2 &&
	    id == needed_id && (needed_version == 0 || version <= needed_version) &&
	    access((p + "/" + fname).c_str(), R_OK) == 0) {
	    if (version > ret)
		ret = version;
	}
    }
    closedir(dir);
    return ret;
}

int transfer_module_t::process_command(const command_t &c) {
    auto remote = cfg.get("persistent") + "/" + c.basename();
    
    switch (c.command) {
    case command_t::INIT:
	last_timestamp = std::chrono::system_clock::now() + std::chrono::seconds(interval);
	return VELOC_SUCCESS;
	
    case command_t::TEST:
	DBG("obtain latest version for " << c.ckpt_name);
	return std::max(get_latest_version(cfg.get("scratch"), c.ckpt_name, c.unique_id, c.version),
			get_latest_version(cfg.get("persistent"), c.ckpt_name, c.unique_id, c.version));
	
    case command_t::CHECKPOINT:
	if (interval > 0) {
	    auto t = std::chrono::system_clock::now();
	    if (t < last_timestamp)
		return VELOC_SUCCESS;
	    else
		last_timestamp = t + std::chrono::seconds(interval);
	}
	DBG("transfer file " << c.ckpt_name << " to " << remote);
	return transfer_file(c.ckpt_name, remote);

    case command_t::RESTART:
	DBG("transfer file " << remote << " to " << c.ckpt_name);
	if (access(c.ckpt_name, R_OK) == 0) {
	    INFO("request to transfer file " << remote << " to " << c.ckpt_name << " ignored as destination already exists");
	    return VELOC_SUCCESS;
	}
	if (access(remote.c_str(), R_OK) != 0) {
	    ERROR("request to transfer file " << remote << " to " << c.ckpt_name << " failed: source does not exist");
	    return VELOC_FAILURE;
	}
	return transfer_file(remote, c.ckpt_name);
	
    default:
	return VELOC_SUCCESS;
    }
}
