#include "ec_module.hpp"
#include "common/status.hpp"

#include "er.h"
#include "common/file_util.hpp"

#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"


ec_module_t::ec_module_t(const config_t &c, MPI_Comm cm) : cfg(c), comm(cm) {
    if (!cfg.get_optional("ec_interval", interval)) {
        INFO("EC interval not specified, every checkpoint will be protected using EC");
	interval = 0;
    }
    int rank, ranks;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &ranks);
    if (ranks < 2) {
        INFO("Running on a single host, EC deactivated");
        interval = -1;
    }
    if (interval < 0)
        return;
    if (ER_Init(cfg.get_cfg_file().c_str()) != ER_SUCCESS)
	FATAL("Failed to initialize ER from config file: " << cfg.get_cfg_file());
    scheme_id = ER_Create_Scheme(comm, std::to_string(rank).c_str(), ranks, 1);
    if (scheme_id < 0) {
        ER_Finalize();
        FATAL("Failed to create scheme using failure domain: " << fdomain);
    }
    MPI_Comm_split(comm, rank, rank, &comm_domain);
}

ec_module_t::~ec_module_t() {
    if (interval >= 0) {
        ER_Free_Scheme(scheme_id);
        ER_Finalize();
        MPI_Comm_free(&comm_domain);
    }
}

int ec_module_t::process_command(const command_t &c) {
    if (interval < 0)
        return VELOC_IGNORED;

    switch (c.command) {
    case command_t::INIT:
	last_timestamp = std::chrono::system_clock::now() + std::chrono::seconds(interval);
	return VELOC_SUCCESS;
    default:
	return VELOC_IGNORED;
    }
}

int ec_module_t::process_commands(const std::vector<command_t> &cmds) {
    if (cmds.size() == 0 || interval < 0)
	return VELOC_IGNORED;

    int version = cmds[0].version, command = cmds[0].command;
    std::string name = cfg.get("scratch") + "/" + cmds[0].name + "-ec-" + std::to_string(version);
    if (command == command_t::CHECKPOINT) {
	if (interval > 0) {
	    auto t = std::chrono::system_clock::now();
	    int checkpoint = 1;
	    if (t < last_timestamp)
		checkpoint = 0;
	    int result;
	    MPI_Allreduce(&checkpoint, &result, 1, MPI_INT, MPI_LAND, comm);
	    if (result)
		last_timestamp = t + std::chrono::seconds(interval);
	    else
		return VELOC_SUCCESS;
	}
	int set_id = ER_Create(comm, comm_domain, name.c_str(), ER_DIRECTION_ENCODE, scheme_id);
	if (set_id == -1) {
	    ERROR("ER_Create failed for checkpoint: " << name);
	    return VELOC_FAILURE;
	}
	for (auto &c : cmds)
	    ER_Add(set_id, c.filename(cfg.get("scratch")).c_str());
        ER_Dispatch(set_id);
        int ret = ER_Wait(set_id);
        ER_Free(set_id);
        if (ret == ER_SUCCESS)
            return VELOC_SUCCESS;
        else {
            ERROR("ER protection failed for checkpoint: " << name);
            return VELOC_FAILURE;
        }
    } else if (command == command_t::RESTART) {
        int local_alive = 1;
	for (auto &c : cmds) {
            DBG("checking local file: " << c.filename(cfg.get("scratch")));
            if (access(c.filename(cfg.get("scratch")).c_str(), R_OK) != 0) {
                local_alive = 0;
                break;
            }
        }
        int result;
        MPI_Allreduce(&local_alive, &result, 1, MPI_INT, MPI_LAND, comm);
        if (result) {
            INFO("skipping ER rebuild, local checkpoints available for: " << name);
            return VELOC_SUCCESS;
        }

        DBG("attempting ER rebuild for checkpoint: " << name);
	int set_id = ER_Create(comm, comm_domain, name.c_str(), ER_DIRECTION_REBUILD, 0);
	if (set_id == -1) {
	    ERROR("ER_Create failed for checkpoint: " << name);
	    return VELOC_FAILURE;
	}
        ER_Dispatch(set_id);
        int ret = ER_Wait(set_id);
        ER_Free(set_id);
        if (ret == ER_SUCCESS)
            return VELOC_SUCCESS;
        else {
            INFO("ER rebuild failed for checkpoint: " << name);
            return VELOC_IGNORED;
        }
    } else
        return VELOC_IGNORED;
}
