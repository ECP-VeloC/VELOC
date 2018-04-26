#include "ec_module.hpp"
#include "common/status.hpp"

#include <limits.h>
#include <unistd.h>
extern "C" {
#include "er.h"
#include "rankstr_mpi.h"
}

#define __DEBUG
#include "common/debug.hpp"

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
    DBG("ER scheme successfully initialized");
}

ec_module_t::~ec_module_t() {
    ER_Free_Scheme(scheme_id);
    ER_Finalize();
    MPI_Comm_free(&comm_domain);
}

int ec_module_t::process_commands(const std::vector<command_t> &cmds) {    
    if (cmds.size() == 0)
	return VELOC_SUCCESS;
    int command = cmds[0].command;
    if (command != command_t::CHECKPOINT && command != command_t::RESTART) {
	ERROR("support for command aggregation for " << command << " not implememnetd yet");
	return VELOC_FAILURE;
    }
    int version = cmds[0].version;
    int set_id;
    if (command == command_t::CHECKPOINT)
	set_id = ER_Create(comm, comm_domain, (cfg.get("scratch") + "/" + std::to_string(version)).c_str(), ER_DIRECTION_ENCODE, scheme_id);
    else
	set_id = ER_Create(comm, comm_domain, (cfg.get("scratch") + "/" + std::to_string(version)).c_str(), ER_DIRECTION_REBUILD, 0);
    for (auto &c : cmds)
	ER_Add(set_id, c.ckpt_name);
    ER_Dispatch(set_id);
    ER_Wait(set_id);
    ER_Free(set_id);
    return VELOC_SUCCESS;
}
