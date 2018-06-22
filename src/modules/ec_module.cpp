#include "ec_module.hpp"
#include "common/status.hpp"

#include <stdexcept>

#include <limits.h>
#include <unistd.h>
#include <dirent.h>

extern "C" {
#include "er.h"
#include "rankstr_mpi.h"
}

#define __DEBUG
#include "common/debug.hpp"

static int get_latest_version(const std::string &p, const std::string &cname, int needed_version) {
    struct dirent *dentry;
    DIR *dir;
    int version, ret = -1;
    
    dir = opendir(p.c_str());
    if (dir == NULL)
	return -1;
    while ((dentry = readdir(dir)) != NULL) {
	std::string fname = std::string(dentry->d_name);
	if (fname.compare(0, cname.length(), cname) == 0 &&
	    access((p + "/" + fname).c_str(), R_OK) == 0 &&
	    sscanf(fname.substr(cname.length()).c_str(), "-%d", &version) == 1 &&
	    (needed_version == 0 || version <= needed_version)) {
	    if (version > ret)
		ret = version;
	}
    }
    closedir(dir);
    return ret;
}

ec_module_t::ec_module_t(const config_t &c, MPI_Comm cm) : cfg(c), comm(cm) {
    if(ER_Init(cfg.get_cfg_file().c_str()) != ER_SUCCESS)
	throw std::runtime_error("Failed to initialize ER from config file: " + cfg.get_cfg_file());
    char host_name[HOST_NAME_MAX] = "";
    gethostname(host_name, HOST_NAME_MAX);
    if(!cfg.get_optional("failure_domain", fdomain))
	fdomain.assign(host_name);
    int rank, ranks;
    MPI_Comm_size(comm, &ranks);
    MPI_Comm_rank(comm, &rank);
     
    scheme_id = ER_Create_Scheme(comm, fdomain.c_str(), ranks, ranks);
    if (scheme_id < 0)
	throw std::runtime_error("Failed to create scheme using failure domain: " + fdomain);
    rankstr_mpi_comm_split(comm, host_name, 0, 0, 1, &comm_domain);
    if (!cfg.get_optional("ec_interval", interval)) {
	INFO("EC interval not specified, every checkpoint will be protected using EC");
	interval = 0;
    }
    DBG("EC scheme successfully initialized");
}

ec_module_t::~ec_module_t() {
    ER_Free_Scheme(scheme_id);
    ER_Finalize();
    MPI_Comm_free(&comm_domain);
}

int ec_module_t::process_command(const command_t &cmd) {
    switch (cmd.command) {
    case command_t::INIT:
	last_timestamp = std::chrono::system_clock::now() + std::chrono::seconds(interval);
	return VELOC_SUCCESS;
	
    case command_t::TEST:
	DBG("get latest EC version for " << cmd.ckpt_name);
	return get_latest_version(cfg.get("scratch"), std::string(cmd.ckpt_name) + "-ec", cmd.version);

    default:
	return VELOC_SUCCESS;
    }
}

int ec_module_t::process_commands(const std::vector<command_t> &cmds) {    
    if (cmds.size() == 0)
	return VELOC_SUCCESS;
    int command = cmds[0].command;
    ASSERT(command == command_t::CHECKPOINT || command == command_t::RESTART);

    int version = cmds[0].version;
    int set_id;
    std::string name = cfg.get("scratch") + "/" + cmds[0].stem() + "-ec-" + std::to_string(version);
    if (command == command_t::CHECKPOINT) {
	if (interval > 0) {
	    auto t = std::chrono::system_clock::now();
	    if (t < last_timestamp)
		return VELOC_SUCCESS;
	    else
		last_timestamp = t + std::chrono::seconds(interval);
	}	
	set_id = ER_Create(comm, comm_domain, name.c_str(), ER_DIRECTION_ENCODE, scheme_id);
	for (auto &c : cmds)
	    ER_Add(set_id, c.ckpt_name);
    } else
	set_id = ER_Create(comm, comm_domain, name.c_str(), ER_DIRECTION_REBUILD, 0);
    int ret = ER_Dispatch(set_id);
    if (ret == ER_SUCCESS)
	ER_Wait(set_id);
    else
	ERROR("EC operation failed with error code: " << ret);
    ER_Free(set_id);
    
    return ret == ER_SUCCESS ? VELOC_SUCCESS : VELOC_FAILURE;
}
