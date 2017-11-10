#define __DEBUG
#include "common/debug.hpp"

#include "include/veloc.h"

#include <unordered_map>
#include <map>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <boost/program_options.hpp>

typedef std::pair<void *, size_t> region_t;
typedef std::map<int, region_t> regions_t;

typedef std::string ckpt_info_t; // for now, just keep checkpoint name
typedef std::unordered_map<int, ckpt_info_t> ckpts_t;

static regions_t mem_regions;
static ckpts_t ckpts;

static std::string scratch_prefix;

void __attribute__ ((constructor)) veloc_constructor() {
    DBG("VELOC constructor");
}

void __attribute__ ((destructor)) veloc_destructor() {
    DBG("VELOC destructor");
}

extern "C" int VELOC_Init(const char *cfg) {
    namespace po = boost::program_options;

    po::options_description desc("VELOC options");
    desc.add_options()
	("help", "This help message")
	("scratch", po::value<std::string>(), "Scratch path for local checkpoints")
    ;
    
    po::variables_map vm;
    std::ifstream ifs(cfg);
    if (ifs.is_open()) {
	po::store(po::parse_config_file(ifs, desc), vm);
	ifs.close();
    } else
	return VELOC_FAILURE;
    
    po::notify(vm);

    if (!vm.count("scratch"))
	return VELOC_FAILURE;

    scratch_prefix = vm["scratch"].as<std::string>();
    return VELOC_SUCCESS;
}

extern "C" void VELOC_Mem_type(VELOCT_type *t, size_t size) {
    *t = size;
}

extern "C" int VELOC_Mem_protect(int id, void *ptr, size_t count, VELOCT_type type) {
    if (mem_regions.find(id) != mem_regions.end()) {
	DBG("Memory region " << id << " already protected");
	return VELOC_FAILURE;
    }
    mem_regions.insert(std::make_pair(id, std::make_pair(ptr, type * count)));
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Mem_unprotect(int id) {
    if (mem_regions.erase(id) > 0)
	return VELOC_SUCCESS;
    else
	return VELOC_FAILURE;
}

extern "C" int VELOC_Checkpoint_begin(int id, const char *name) {
    if (ckpts.find(id) != ckpts.end()) {
	DBG("Checkpoint " << id << " already initiated");
	return VELOC_FAILURE;
    }
    ckpts.insert(std::make_pair(id, std::string(name)));
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Checkpoint_mem(int id, int /* level */) {
    auto it = ckpts.find(id);
    if (it == ckpts.end())
	return VELOC_FAILURE;
    std::string cname = scratch_prefix + "/" + it->second + ".dat";
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

extern "C" int VELOC_Checkpoint_end(int id, int success) {
    if (ckpts.erase(id) > 0)
	return VELOC_SUCCESS;
    else
	return VELOC_FAILURE;
}

extern "C" int VELOC_Finalize() {
    return VELOC_SUCCESS;
}
