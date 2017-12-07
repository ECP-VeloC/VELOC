#include "client.hpp"
#include "include/veloc.h"

#include <fstream>
#include <stdexcept>

#define __DEBUG
#include "common/debug.hpp"

veloc_client_t::veloc_client_t(int r, const char *cfg_file) : rank(r) {
    if (!cfg.get_parameters(cfg_file))
	throw std::runtime_error("configuration error, cannot initialize VELOC");
    if (cfg.is_sync())
	modules = new module_manager_t();
    else {
	queue = new veloc_ipc::shm_queue_t<command_t>();
	queue->set_id(std::to_string(r));
    }
    DBG("VELOC initialized");
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
    std::ostringstream os;
    os << name << "-" << rank << "-" << version;
    return command_t(cmd, version, cfg.get_scratch() + bf::path::preferred_separator + os.str() + ".dat");
}

bool veloc_client_t::checkpoint_begin(const char *name, int version) {
    if (checkpoint_in_progress) {
	ERROR("nested checkpoints not yet supported");
	return false;
    }
    current_ckpt = gen_ckpt_details(command_t::CHECKPOINT_END, name, version);
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
	modules->run(std::to_string(rank), current_ckpt);
    else
	queue->enqueue(current_ckpt);	
    return true;
}

int veloc_client_t::restart_test(const char *name) {
    std::string cname(name);
    int ret = VELOC_FAILURE;
    for(auto& f : boost::make_iterator_range(bf::directory_iterator(cfg.get_scratch()), {})) {
	std::string fname = f.path().leaf().string();
	int ckpt_rank, version;
	if (bf::is_regular_file(f.path()) &&
	    fname.compare(0, cname.length(), cname) == 0 &&
	    sscanf(fname.substr(cname.length()).c_str(), "-%d-%d", &ckpt_rank, &version) == 2 &&
	    ckpt_rank == rank) {	 
	    if (version > ret)
		ret = version;
	}
    }
    // TO-DO: allreduce on versions to select min of max available on all ranks (maybe separate function test_all)
    return ret;
}

std::string veloc_client_t::route_file() {
    return std::string(current_ckpt.ckpt_name);
}

bool veloc_client_t::restart_begin(const char *name, int version) {
    if (checkpoint_in_progress) {
	INFO("cannot restart while checkpoint in progress");
	return false;
    }
    current_ckpt = gen_ckpt_details(command_t::RESTART_BEGIN, name, version);
    return bf::exists(current_ckpt.ckpt_name);
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
