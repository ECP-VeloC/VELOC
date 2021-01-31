#include "client.hpp"
#include "common/file_util.hpp"

#include <fstream>
#include <stdexcept>
#include <regex>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

//#define __DEBUG
#include "common/debug.hpp"

static bool validate_name(const std::string &name) {
    std::regex e("[a-zA-Z0-9_\\.]+");
    return std::regex_match(name, e);
}

static void launch_backend(const std::string &cfg_file) {
    char *path = getenv("VELOC_BIN");
    std::string command;
    if (path != NULL)
        command = std::string(path) + "/";
    command += "veloc-backend " + cfg_file + " --disable-ec > /dev/null";
    if (system(command.c_str()) != 0)
        FATAL("cannot launch active backend for async mode, error: " << strerror(errno));
}

client_impl_t::client_impl_t(unsigned int id, const std::string &cfg_file) :
    cfg(cfg_file), collective(false), rank(id) {
    if (cfg.is_sync()) {
	modules = new module_manager_t();
	modules->add_default_modules(cfg);
    } else {
        launch_backend(cfg_file);
	queue = new comm_client_t<command_t>(rank);
    }
    ec_active = run_blocking(command_t(rank, command_t::INIT, 0, "")) > 0;
    DBG("VELOC initialized");
}

client_impl_t::client_impl_t(MPI_Comm c, const std::string &cfg_file) :
    cfg(cfg_file), comm(c), collective(true) {
    MPI_Comm_rank(comm, &rank);
    if (cfg.is_sync()) {
	modules = new module_manager_t();
	modules->add_default_modules(cfg, comm, true);
    } else {
        launch_backend(cfg_file);
	queue = new comm_client_t<command_t>(rank);
    }
    ec_active = run_blocking(command_t(rank, command_t::INIT, 0, "")) > 0;
    DBG("VELOC initialized");
}

client_impl_t::~client_impl_t() {
    delete queue;
    delete modules;
    DBG("VELOC finalized");
}

bool client_impl_t::checkpoint_wait() {
    if (cfg.is_sync())
	return true;
    if (checkpoint_in_progress) {
	ERROR("need to finalize local checkpoint first by calling checkpoint_end()");
	return false;
    }
    return queue->wait_completion() == VELOC_SUCCESS;
}

bool client_impl_t::checkpoint(const std::string &name, int version) {
    return checkpoint_wait()
        && checkpoint_begin(name, version)
        && checkpoint_mem(VELOC_CKPT_ALL, {})
        && checkpoint_end(true);
}

bool client_impl_t::checkpoint_begin(const std::string &name, int version) {
    if (checkpoint_in_progress) {
	ERROR("nested checkpoints not yet supported");
	return false;
    }
    if (!validate_name(name) || version < 0) {
	ERROR("checkpoint name and/or version incorrect: name can only include [a-zA-Z0-9_] characters, version needs to be non-negative integer");
	return false;
    }

    DBG("called checkpoint_begin");
    current_ckpt = command_t(rank, command_t::CHECKPOINT, version, name.c_str());
    checkpoint_in_progress = true;
    return true;
}

bool client_impl_t::checkpoint_mem(int mode, const std::set<int> &ids) {
    if (!checkpoint_in_progress) {
	ERROR("must call checkpoint_begin() first");
	return false;
    }
    regions_t ckpt_regions;
    if (mode == VELOC_CKPT_ALL)
        ckpt_regions = mem_regions;
    else if (mode == VELOC_CKPT_SOME) {
        for (auto it = ids.begin(); it != ids.end(); it++) {
            auto found = mem_regions.find(*it);
            if (found != mem_regions.end())
                ckpt_regions.insert(*found);
        }
    } else if (mode == VELOC_CKPT_REST) {
        ckpt_regions = mem_regions;
        for (auto it = ids.begin(); it != ids.end(); it++)
            ckpt_regions.erase(*it);
    }
    if (ckpt_regions.size() == 0) {
	ERROR("empty selection of memory regions to checkpoint, please check protection and/or selective checkpointing primitives");
	return false;
    }

    std::ofstream f;
    f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
	size_t regions_size = ckpt_regions.size();
        size_t header_size = sizeof(size_t) + regions_size * (sizeof(int) + sizeof(size_t));
        // write data and determine serialized sizes
        f.seekp(header_size);
        for (auto &e : ckpt_regions) {
            region_t &info = e.second;
            if (info.ptr != NULL)
                f.write((char *)info.ptr, info.size);
            else {
                size_t start = f.tellp();
                info.s(f);
                info.size = (size_t)f.tellp() - start;
            }
        }
        // write header
        f.seekp(0);
	f.write((char *)&regions_size, sizeof(size_t));
	for (auto &e : ckpt_regions) {
	    f.write((char *)&(e.first), sizeof(int));
	    f.write((char *)&(e.second.size), sizeof(size_t));
	}
    } catch (std::ofstream::failure &f) {
	ERROR("cannot write to checkpoint file: " << current_ckpt << ", reason: " << f.what());
	return false;
    }
    return true;
}

bool client_impl_t::checkpoint_end(bool /*success*/) {
    checkpoint_in_progress = false;
    if (cfg.is_sync())
	return modules->notify_command(current_ckpt) == VELOC_SUCCESS;
    else {
	queue->enqueue(current_ckpt);
	return true;
    }
}

int client_impl_t::run_blocking(const command_t &cmd) {
    if (cfg.is_sync())
	return modules->notify_command(cmd);
    else {
	queue->enqueue(cmd);
	return queue->wait_completion();
    }
}

int client_impl_t::restart_test(const std::string &name, int needed_version) {
    if (!validate_name(name) || needed_version < 0) {
	ERROR("checkpoint name and/or version incorrect: name can only include [a-zA-Z0-9_] characters, version needs to be non-negative integer");
	return VELOC_FAILURE;
    }
    int version = run_blocking(command_t(rank, command_t::TEST, needed_version, name.c_str()));
    DBG(name << ": latest version = " << version);
    if (collective) {
	int min_version;
	MPI_Allreduce(&version, &min_version, 1, MPI_INT, MPI_MIN, comm);
	return min_version;
    } else
	return version;
}

std::string client_impl_t::route_file(const std::string &original) {
    char abs_path[PATH_MAX + 1];
    if (original[0] != '/' && getcwd(abs_path, PATH_MAX) != NULL)
	current_ckpt.assign_path(current_ckpt.original, std::string(abs_path) + "/" + original);
    else
	current_ckpt.assign_path(current_ckpt.original, original);
    return current_ckpt.filename(cfg.get("scratch"));
}

bool client_impl_t::restart(const std::string &name, int version) {
    return restart_begin(name, version)
        && recover_mem(VELOC_CKPT_ALL, {})
        && restart_end(true);
}

bool client_impl_t::restart_begin(const std::string &name, int version) {
    if (checkpoint_in_progress) {
	INFO("cannot restart while checkpoint in progress");
	return false;
    }
    if (!validate_name(name) || version < 0) {
	ERROR("checkpoint name and/or version incorrect: name can only include [a-zA-Z0-9_] characters, version needs to be non-negative integer");
	return VELOC_FAILURE;
    }

    int result, end_result;
    current_ckpt = command_t(rank, command_t::RESTART, version, name.c_str());
    result = run_blocking(current_ckpt);
    if (collective)
	MPI_Allreduce(&result, &end_result, 1, MPI_INT, MPI_LOR, comm);
    else
	end_result = result;
    if (end_result == VELOC_SUCCESS) {
        header_size = 0;
	return true;
    } else
	return false;
}

bool client_impl_t::read_header() {
    region_info.clear();
    try {
	std::ifstream f;
        size_t expected_size = 0;

	f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	f.open(current_ckpt.filename(cfg.get("scratch")), std::ifstream::in | std::ifstream::binary);
	size_t no_regions, region_size;
	int id;
	f.read((char *)&no_regions, sizeof(size_t));
	for (unsigned int i = 0; i < no_regions; i++) {
	    f.read((char *)&id, sizeof(int));
	    f.read((char *)&region_size, sizeof(size_t));
	    region_info.insert(std::make_pair(id, region_size));
            expected_size += region_size;
	}
	header_size = f.tellg();
        f.seekg(0, f.end);
        size_t file_size = (size_t)f.tellg() - header_size;
        if (file_size != expected_size)
            throw std::ifstream::failure("file size " + std::to_string(file_size) + " does not match expected size " + std::to_string(expected_size));
    } catch (std::ifstream::failure &e) {
	ERROR("cannot validate header for checkpoint " << current_ckpt << ", reason: " << e.what());
	header_size = 0;
	return false;
    }
    return true;
}

size_t client_impl_t::recover_size(int id) {
    if (header_size == 0)
        read_header();
    auto it = region_info.find(id);
    if (it == region_info.end())
	return 0;
    else
	return it->second;
}

bool client_impl_t::recover_mem(int mode, const std::set<int> &ids) {
    if (header_size == 0 && !read_header()) {
	ERROR("cannot recover in memory mode if header unavailable or corrupted");
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
            auto it = mem_regions.find(e.first);
            if (it == mem_regions.end()) {
		ERROR("no protected memory region defined for id " << e.first);
		return false;
	    }
            region_t &info = it->second;
            if (info.ptr != NULL) { // direct read of raw data
                if (info.size < e.second) {
                    ERROR("protected memory region " << e.first << " is too small ("
                          << info.size << ") to hold required size ("
                          << e.second << ")");
                    return false;
                }
                f.read((char *)info.ptr, e.second);
            } else { // deserialize
                if (!info.d(f)) {
                    ERROR("protected data structure " << e.first << " could not be deserialized");
                    return false;
                }
            }
	}
    } catch (std::ifstream::failure &e) {
	ERROR("cannot read checkpoint file " << current_ckpt << ", reason: " << e.what());
	return false;
    }
    return true;
}

bool client_impl_t::restart_end(bool /*success*/) {
    return true;
}
