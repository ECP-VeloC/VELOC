#include "client.hpp"
#include "include/veloc.h"

#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <ftw.h>
#include <limits.h>
#include <stdlib.h>

//#define __DEBUG
#include "common/debug.hpp"

veloc_client_t::veloc_client_t(unsigned int id, const char *cfg_file) :
    cfg(cfg_file), collective(false), rank(id) {
    if (!cfg.get_optional("max_versions", max_versions)) {
	INFO("Max number of versions to keep not specified, keeping all");
	max_versions = 0;
    }
    if (cfg.is_sync()) {
	modules = new module_manager_t();
	modules->add_default_modules(cfg);
    } else
	queue = new veloc_ipc::shm_queue_t<command_t>(std::to_string(rank).c_str());
    ec_active = run_blocking(command_t(rank, command_t::INIT, 0, "")) > 0;
    DBG("VELOC initialized");
}

veloc_client_t::veloc_client_t(MPI_Comm c, const char *cfg_file) :
    cfg(cfg_file), comm(c), collective(true) {
    MPI_Comm_rank(comm, &rank);
    if (!cfg.get_optional("max_versions", max_versions)) {
	INFO("Max number of versions to keep not specified, keeping all");
	max_versions = 0;
    }
    if (cfg.is_sync()) {
	modules = new module_manager_t();
	modules->add_default_modules(cfg);
    } else
	queue = new veloc_ipc::shm_queue_t<command_t>(std::to_string(rank).c_str());
    ec_active = run_blocking(command_t(rank, command_t::INIT, 0, "")) > 0;
    DBG("VELOC initialized");
}


static int rm_file(const char *f, const struct stat *sbuf, int type, struct FTW *ftwb) {
    return remove(f);
}

void veloc_client_t::cleanup() {
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

int veloc_client_t::backend_ready() {
    if (cfg.is_sync()) return true;
    if (checkpoint_in_progress) {
	ERROR("need to finalize local checkpoint first by calling checkpoint_end()");
	return false;
    }
    return queue->check_completion();
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
    current_ckpt = command_t(rank, command_t::CHECKPOINT, version, name);
    // remove old versions (only if EC is not active)
    if (!ec_active && max_versions > 0) {
	DBG("remove old versions");
	auto &version_history = checkpoint_history[name];
	version_history.push_back(version);
	if ((int)version_history.size() > max_versions) {
	    // wait for operations to complete in async mode before deleting old versions
	    if (!cfg.is_sync())
		queue->wait_completion(false);
	    remove(current_ckpt.filename(cfg.get("scratch"), version_history.front()).c_str());
	    version_history.pop_front();
	}
    }
    checkpoint_in_progress = true;
    return true;
}

bool veloc_client_t::checkpoint_mem() {
    if (!checkpoint_in_progress) {
	ERROR("must call checkpoint_begin() first");
	return false;
    }
    std::ofstream f;
    f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
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
    std::perror("WERROR: ");
	return false;
    }
    return true;
}

bool veloc_client_t::checkpoint_end(bool /*success*/) {
    checkpoint_in_progress = false;
    if (cfg.is_sync())
	return modules->notify_command(current_ckpt) == VELOC_SUCCESS;
    else {
	queue->enqueue(current_ckpt);
	return true;
    }
}

int veloc_client_t::run_blocking(const command_t &cmd) {
    if (cfg.is_sync())
	return modules->notify_command(cmd);
    else {
	queue->enqueue(cmd);
	return queue->wait_completion();
    }
}

int veloc_client_t::restart_test(const char *name, int needed_version) {
    int version = run_blocking(command_t(rank, command_t::TEST, needed_version, name));
    DBG(name << ": latest version = " << version);
    if (collective) {
	int min_version;
	MPI_Allreduce(&version, &min_version, 1, MPI_INT, MPI_MIN, comm);
	return min_version;
    } else
	return version;
}

std::string veloc_client_t::route_file(const char *original) {
    char abs_path[PATH_MAX + 1];
    if (original[0] != '/' && getcwd(abs_path, PATH_MAX) != NULL)
	current_ckpt.assign_path(abs_path, std::string(abs_path) + "/" + std::string(original));
    return current_ckpt.filename(cfg.get("scratch"));    	
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
	result = run_blocking(current_ckpt);
    if (collective)
	MPI_Allreduce(&result, &end_result, 1, MPI_INT, MPI_LOR, comm);
    else
	end_result = result;
    if (end_result == VELOC_SUCCESS) {
	if (!ec_active && max_versions > 0) {
	    auto &version_history = checkpoint_history[name];
	    version_history.clear();
	    version_history.push_back(version);
	}
	return read_header();
    } else
	return false;
}

bool veloc_client_t::read_header() {
    region_info.clear();
    try {
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ifstream::in | std::ifstream::binary);
	size_t no_regions, region_size;
	int id;
	f.read((char *)&no_regions, sizeof(size_t));
	for (unsigned int i = 0; i < no_regions; i++) {
	    f.read((char *)&id, sizeof(int));
	    f.read((char *)&region_size, sizeof(size_t));
	    region_info.insert(std::make_pair(id, region_size));
	}
	header_size = f.tellg();
    } catch (std::ifstream::failure &e) {
	ERROR("cannot read checkpoint file " << current_ckpt << ", reason: " << e.what());
	header_size = 0;
	return false;
    }
    return true;
}

size_t veloc_client_t::recover_size() {
  return region_info.size();
}

size_t veloc_client_t::recover_size(int id) {
    auto it = region_info.find(id);
    if (it == region_info.end())
	return 0;
    else
	return it->second;
}

bool veloc_client_t::recover_mem(int mode, std::set<int> &ids) {
    if (header_size == 0) {
	ERROR("cannot recover before successful restart begin");
	return false;
    }	
    try {
	std::ifstream f;
	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ifstream::in | std::ifstream::binary);
	f.seekg(header_size);
	for (auto &e : region_info) {
	    bool found = ids.find(e.first) != ids.end();
	    if ((mode == VELOC_RECOVER_SOME && !found) || (mode == VELOC_RECOVER_REST && found)) {
		f.seekg(e.second, std::ifstream::cur);
		continue;
	    }
	    if (mem_regions.find(e.first) == mem_regions.end()) {
		ERROR("no protected memory region defined for id " << e.first);
		return false;
	    }
	    if (mem_regions[e.first].second < e.second) {
		ERROR("protected memory region " << e.first << " is too small ("
		      << mem_regions[e.first].second << ") to hold required size ("
		      << e.second << ")");
		return false;
	    }
	    f.read((char *)mem_regions[e.first].first, e.second);
	}
    } catch (std::ifstream::failure &e) {
	ERROR("cannot read checkpoint file " << current_ckpt << ", reason: " << e.what());
    perror("Error printout: ");
	return false;
    }
    return true;
}

bool veloc_client_t::restart_end(bool /*success*/) {
    return true;
}
