#include "client.hpp"
#include "include/veloc.h"

#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <ftw.h>

#define __DEBUG
#include "common/debug.hpp"

veloc_client_t::veloc_client_t(int r, const char *cfg_file) : cfg(cfg_file), rank(r) {
    if (cfg.is_sync()) {
	modules = new module_manager_t();
	modules->add_default_modules(cfg);
	modules->notify_command(command_t(rank, command_t::INIT, 0, ""));
    } else {
	queue = new veloc_ipc::shm_queue_t<command_t>(std::to_string(r).c_str());
	queue->enqueue(command_t(rank, command_t::INIT, 0, ""));
    }
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

command_t veloc_client_t::gen_ckpt_details(int cmd, const char *name, int version) {
    return command_t(rank, cmd, version, cfg.get("scratch") + "/" + std::string(name) +
		     "-" + std::to_string(rank) + "-" + std::to_string(version) + ".dat");
}

bool veloc_client_t::checkpoint_wait() {
    if (cfg.is_sync()) {
	INFO("waiting for a checkpoint in sync mode is not necessary, result is returned by checkpoint_end() directly");
	return true;
    }    
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
    current_ckpt = gen_ckpt_details(command_t::CHECKPOINT, name, version);
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
	f.open(current_ckpt.ckpt_name, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
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

int veloc_client_t::restart_test(const char *name) {
    return run_blocking(command_t(rank, command_t::TEST, 0, name));
}

std::string veloc_client_t::route_file() {
    return std::string(current_ckpt.ckpt_name);
}

bool veloc_client_t::restart_begin(const char *name, int version) {
    if (checkpoint_in_progress) {
	INFO("cannot restart while checkpoint in progress");
	return false;
    }
    current_ckpt = gen_ckpt_details(command_t::RESTART, name, version);
    if (access(current_ckpt.ckpt_name, R_OK) == 0)
	return true;
    else
	return run_blocking(current_ckpt) == VELOC_SUCCESS;
}

bool veloc_client_t::recover_mem(int mode, std::set<int> &ids) {
    if (mode != VELOC_RECOVER_ALL) {
	ERROR("only VELOC_RECOVER_ALL mode currently supported");
	return false;
    }

    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
	f.open(current_ckpt.ckpt_name, std::ios_base::in | std::ios_base::binary);
	size_t no_regions, region_size;
	int id;
	f.read((char *)&no_regions, sizeof(size_t));
	for (unsigned int i = 0; i < no_regions; i++) {
	    f.read((char *)&id, sizeof(int));
	    f.read((char *)&region_size, sizeof(size_t));
	    if (mem_regions.find(id) == mem_regions.end() || mem_regions[id].second != region_size) {
		ERROR("protected memory region " << i << " does not exist or match size in recovery checkpoint");
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
