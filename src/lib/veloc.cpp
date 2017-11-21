#include <unordered_map>
#include <map>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "common/config.hpp"
#include "include/veloc.h"

#define __DEBUG
#include "common/debug.hpp"

typedef std::pair<void *, size_t> region_t;
typedef std::map<int, region_t> regions_t;

typedef std::string ckpt_info_t; // for now, just keep checkpoint name
typedef std::unordered_map<int, ckpt_info_t> ckpts_t;

static regions_t mem_regions;
static ckpts_t ckpts;

namespace bf = boost::filesystem;

static bf::path restart_ckpt;
static int rank;
static config_t cfg;

void __attribute__ ((constructor)) veloc_constructor() {
}

void __attribute__ ((destructor)) veloc_destructor() {
}

extern "C" int VELOC_Init(int r, const char *cfg_file) {
    DBG("VELOC initialization");
    rank = r;
    if (cfg.get_parameters(std::string(cfg_file)))
	return VELOC_SUCCESS;
    else
	return VELOC_FAILURE;
}
    
extern "C" int VELOC_Mem_protect(int id, void *ptr, size_t count, size_t base_size) {
    mem_regions[id] = std::make_pair(ptr, base_size * count);
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Mem_unprotect(int id) {
    if (mem_regions.erase(id) > 0)
	return VELOC_SUCCESS;
    else
	return VELOC_FAILURE;
}

extern "C" int VELOC_Checkpoint_begin(const char *name, int version) {
    if (ckpts.find(version) != ckpts.end()) {
	DBG("Checkpoint " << version << " already initiated");
	return VELOC_FAILURE;
    }
    std::ostringstream os;
    os << name << "-" << rank << "-" << version;
    ckpts.insert(std::make_pair(version, os.str()));
    
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Checkpoint_mem(int version) {
    auto it = ckpts.find(version);
    if (it == ckpts.end())
	return VELOC_FAILURE;
    bf::path cname(cfg.get_scratch() + bf::path::preferred_separator + it->second + ".dat");
    int fd = open(cname.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IREAD | S_IWRITE);
    if (fd == -1) {
	ERROR("can't open checkpoint file " << cname);
	return VELOC_FAILURE;
    }
    for (auto it = mem_regions.begin(); it != mem_regions.end(); ++it) {
	void *ptr = it->second.first;
	size_t size = it->second.second;
	size_t written = 0;
	while (written < size) {
	   ssize_t ret = write(fd, ((char *)ptr + written), size - written);
	   if (ret == -1) {
	       ERROR("can't write to checkpoint file " << cname);
	       return VELOC_FAILURE;
	   }
	   written += ret;
	}
    }
    close(fd);

    return VELOC_SUCCESS;
}

extern "C" int VELOC_Checkpoint_end(int version, int success) { 
    if (ckpts.erase(version) > 0)
	return VELOC_SUCCESS;
    else
	return VELOC_FAILURE;
}

extern "C" int VELOC_Restart_test(const char *name) {
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

extern "C" int VELOC_Restart_begin(const char *name, int version) {
    std::ostringstream os;
    os << name << "-" << rank << "-" << version;
    bf::path cname(cfg.get_scratch() + bf::path::preferred_separator + os.str() + ".dat");
    
    if (!bf::is_regular_file(cname))
	return VELOC_FAILURE;

    restart_ckpt = cname;
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Restart_mem(int version) {
    int fd = open(restart_ckpt.string().c_str(), O_RDONLY);
    if (fd == -1) {
	ERROR("can't open checkpoint file " << restart_ckpt);
	return VELOC_FAILURE;
    }
    for (auto it = mem_regions.begin(); it != mem_regions.end(); ++it) {
	void *ptr = it->second.first;
	size_t size = it->second.second;
	size_t bytes_read = 0;
	while (bytes_read < size) {
	   ssize_t ret = read(fd, ((char *)ptr + bytes_read), size - bytes_read);
	   if (ret == -1) {
	       ERROR("can't read from checkpoint file " << restart_ckpt.string());
	       return VELOC_FAILURE;
	   }
	   bytes_read += ret;
	}
    }
    close(fd);
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Restart_end(int version, int success) {
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Finalize() {
    DBG("VELOC finalized");
    return VELOC_SUCCESS;
}
