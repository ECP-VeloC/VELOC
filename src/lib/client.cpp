#include "client.hpp"
#include "include/veloc.h"

#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <ftw.h>

//#define __DEBUG
#include "common/debug.hpp"

veloc_client_t::veloc_client_t(MPI_Comm c, const char *cfg_file) :
    cfg(cfg_file), comm(c) {
    MPI_Comm_rank(comm, &rank);
    if (!cfg.get_optional("max_versions", max_versions)) {
	INFO("Max number of versions to keep not specified, keeping all");
	max_versions = 0;
    }
    collective = cfg.get_optional("collective", true);
    if (cfg.is_sync()) {
	modules = new module_manager_t();
	modules->add_default_modules(cfg, comm, true);
    } else
	queue = new veloc_ipc::shm_queue_t<command_t>(std::to_string(rank).c_str());
    ec_active = dispatch_command(command_t(rank, command_t::INIT, 0, ""), true) > 0;
    DBG("VELOC initialized");
}

static int rm_file(const char *f, const struct stat *sbuf, int type, struct FTW *ftwb) {
    return remove(f);
}

void veloc_client_t::cleanup() {
    // TODO: Does not clean up EC files. Needs to be moved to the active backend.
    // Maybe a separate cleanup module
    nftw(cfg.get("scratch").c_str(), rm_file, 128, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
    nftw(cfg.get("persistent").c_str(), rm_file, 128, FTW_DEPTH | FTW_MOUNT | FTW_PHYS);
}

veloc_client_t::~veloc_client_t() {
    delete queue;
    delete modules;
    DBG("VELOC finalized");
}

bool veloc_client_t::mem_protect(int id, void *ptr, size_t count, size_t base_size) {
    mem_regions[id] = std::make_pair(ptr, base_size * count);
    return true;
}

bool veloc_client_t::mem_unprotect(int id) {
    return mem_regions.erase(id) > 0;
}

bool veloc_client_t::checkpoint_wait() {
    if (cfg.is_sync())
	return true;
    if (checkpoint_in_progress) {
	ERROR("need to finalize local checkpoint first by calling checkpoint_end()");
	return false;
    }
    return queue->wait_completion() == VELOC_SUCCESS;
}

bool veloc_client_t::checkpoint_begin(const char *name, int version) {
    if (checkpoint_in_progress) {
	ERROR("nested checkpoints not yet supported");
	return false;
    }
    if (version < 0) {
	ERROR("checkpoint version needs to be non-negative integer");
	return false;
    }
    current_ckpt = command_t(rank, command_t::CHECKPOINT_BEGIN, version, name);
    checkpoint_in_progress = true;
    return dispatch_command(current_ckpt, false) == VELOC_SUCCESS;
}

bool veloc_client_t::checkpoint_mem() {
    if (!checkpoint_in_progress) {
	ERROR("must call checkpoint_begin() first");
	return false;
    }
    std::ofstream f;
    f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	size_t regions_size = mem_regions.size();
	f.write((char *)&regions_size, sizeof(size_t));
	for (auto &e : mem_regions) {
	    f.write((char *)&(e.first), sizeof(int));
	    f.write((char *)&(e.second.second), sizeof(size_t));
	}
        for (auto &e : mem_regions)
	    f.write((char *)e.second.first, e.second.second);
    } catch (std::ofstream::failure &f) {
	ERROR("cannot write to checkpoint file: " << current_ckpt << ", reason: " << f.what());
	return false;
    }
    current_ckpt.command = CHECKPOINT_CHUNK;
    return dispatch_command(current_ckpt, false) == VELOC_SUCCESS;
}

bool veloc_client_t::checkpoint_end(bool /*success*/) {
    current_ckpt.command = CHECKPOINT_CHUNK;
    for (f: route_queue) {
	strncpy(current_ckpt.name, f.first.c_str(), f.first.length());
	strncpy(current_ckpt.original, f.second.c_str(), f.second.length());
	dispatch_command(current_ckpt, false);
    }
    route_queue.clear();      
    checkpoint_in_progress = false;
    current_ckpt.command = CHECKPOINT_END;
    return dispatch_command(current_ckpt, false) == VELOC_SUCCESS;
}

int veloc_client_t::dispatch_command(const command_t &cmd, bool blocking) {
    if (cfg.is_sync())
	return modules->notify_command(cmd);
    else {
	queue->enqueue(cmd);
	if (blocking)
	    return queue->wait_completion();
	else
	    return VELOC_SUCCESS;
    }
}

int veloc_client_t::restart_test(const char *name, int needed_version) {
    int version = dispatch_command(command_t(rank, command_t::TEST, needed_version, name), true);
    if (collective) {
	int min_version;
	MPI_Allreduce(&version, &min_version, 1, MPI_INT, MPI_MIN, comm);
	return min_version;
    } else
	return version;
}

std::string veloc_client_t::route_file(const char *original) {
    if (checkpoint_in_progress) {
	// add file to checkpointing list
	route_queue.push_back(original);
	current_ckpt = command_t(rank, command_t::CHECKPOINT, version, name);
    }

    std::strncpy(current_ckpt.original, original, command_t::MAX_SIZE);
    return std::string(current_ckpt.filename(cfg.get("scratch")));
}

bool veloc_client_t::restart_begin(const char *name, int version) {
    int result, end_result;
    
    if (checkpoint_in_progress) {
	INFO("cannot restart while checkpoint in progress");
	return false;
    }
    current_ckpt = command_t(rank, command_t::RESTART, version, name);    
    if (access(current_ckpt.filename(cfg.get("scratch")).c_str(), R_OK) == 0)
	result = VELOC_SUCCESS;
    else 
	result = dispatch_command(current_ckpt, true);
    if (collective)
	MPI_Allreduce(&result, &end_result, 1, MPI_INT, MPI_LOR, comm);
    else
	end_result = result;
    return end_result == VELOC_SUCCESS;
}

bool veloc_client_t::recover_mem(int mode, std::set<int> &ids) {
    if (mode != VELOC_RECOVER_ALL) {
	ERROR("only VELOC_RECOVER_ALL mode currently supported");
	return false;
    }

    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ios_base::in | std::ios_base::binary);
	size_t no_regions, region_size;
	int id;
	f.read((char *)&no_regions, sizeof(size_t));
	for (unsigned int i = 0; i < no_regions; i++) {
	    f.read((char *)&id, sizeof(int));
	    f.read((char *)&region_size, sizeof(size_t));
	    if (mem_regions.find(id) == mem_regions.end()) {
		ERROR("protected memory region " << id << " does not exist");
		return false;
	    }
	    if (mem_regions[id].second != region_size) {
		ERROR("protected memory region " << id << " has size " << region_size
		      << " instead of expected " << mem_regions[id].second);
		return false;
	    }
	}
	for (auto &e : mem_regions)
	    f.read((char *)e.second.first, e.second.second);
    } catch (std::ifstream::failure &e) {
	ERROR("cannot read checkpoint file " << current_ckpt << ", reason: " << e.what());
	return false;
    }
    return true;
}

bool veloc_client_t::restart_end(bool /*success*/) {
    return true;
}
