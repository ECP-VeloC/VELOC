#include "client.hpp"
#include "common/config.hpp"
#include "include/veloc.h"

#include <fstream>
#include <stdexcept>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define __DEBUG
#include "common/debug.hpp"

veloc_client_t::veloc_client_t(int r, const char *cfg_file) : rank(r), checkpoint_in_progress(false) {
    if (!cfg.get_parameters(cfg_file))
	throw std::runtime_error("cannot initialize VELOC");
    DBG("VELOC initialized");
}

bool veloc_client_t::mem_protect(int id, void *ptr, size_t count, size_t base_size) {
    mem_regions[id] = std::make_pair(ptr, base_size * count);
    return true;
}

bool veloc_client_t::mem_unprotect(int id) {
    return mem_regions.erase(id) > 0;
}

bf::path veloc_client_t::gen_ckpt_name(const char *name, int version) {
    std::ostringstream os;    
    os << name << "-" << rank << "-" << version;
    return bf::path(cfg.get_scratch() + bf::path::preferred_separator + os.str() + ".dat");    
}

bool veloc_client_t::checkpoint_begin(const char *name, int version) {
    if (checkpoint_in_progress) {
	ERROR("nested checkpoints not yet supported");
	return false;
    }
    current_ckpt = gen_ckpt_name(name, version);
    checkpoint_in_progress = true;
    return true;
}

bool veloc_client_t::checkpoint_mem() {
    if (!checkpoint_in_progress) {
	ERROR("must call checkpoint_being() first");
	return false;
    }
    int fd = open(current_ckpt.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
    if (fd == -1) {
	ERROR("can't open checkpoint file " << current_ckpt);
	return false;
    }
    for (auto it = mem_regions.begin(); it != mem_regions.end(); ++it) {
	void *ptr = it->second.first;
	size_t size = it->second.second;
	size_t written = 0;
	while (written < size) {
	   ssize_t ret = write(fd, ((char *)ptr + written), size - written);
	   if (ret == -1) {
	       ERROR("can't write to checkpoint file " << current_ckpt);
	       return false;
	   }
	   written += ret;
	}
    }
    close(fd);

    return true;
}

bool veloc_client_t::checkpoint_end(bool /*success*/) {
    checkpoint_in_progress = false;
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

std::string veloc_client_t::route_file(const char *name, int version) {
    return gen_ckpt_name(name, version).string();
}

bool veloc_client_t::restart_begin(const char *name, int version) {
    if (checkpoint_in_progress) {
	INFO("cannot restart while checkpoint in progress");
	return false;
    }
    current_ckpt = gen_ckpt_name(name, version);    
    return bf::is_regular_file(current_ckpt);
}

bool veloc_client_t::restart_mem() {
    int fd = open(current_ckpt.c_str(), O_RDONLY);
    if (fd == -1) {
	ERROR("can't open checkpoint file " << current_ckpt);
	return false;
    }
    for (auto it = mem_regions.begin(); it != mem_regions.end(); ++it) {
	void *ptr = it->second.first;
	size_t size = it->second.second;
	size_t bytes_read = 0;
	while (bytes_read < size) {
	   ssize_t ret = read(fd, ((char *)ptr + bytes_read), size - bytes_read);
	   if (ret == -1) {
	       ERROR("can't read from checkpoint file " << current_ckpt.string());
	       return false;
	   }
	   bytes_read += ret;
	}
    }
    close(fd);
    return true;
}

bool veloc_client_t::restart_end(bool /*success*/) {
    return true;
}

veloc_client_t::~veloc_client_t() {
    DBG("VELOC finalized");
}
