#include "client.hpp"
#include "common/file_util.hpp"
#include "common/ckpt_util.hpp"
#include "backend/work_queue.hpp"

#include <fstream>
#include <stdexcept>
#include <regex>
#include <future>
#include <queue>

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

//#define __DEBUG
#include "common/debug.hpp"

static inline bool validate_name(const std::string &name) {
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

bool client_impl_t::check_threaded() {
    bool threaded = cfg.get_bool("threaded", false);
    if (threaded) {
	int provided;
        MPI_Query_thread(&provided);
        if (provided != MPI_THREAD_MULTIPLE)
            FATAL("MPI threaded mode requested but not available, please use MPI_Init_thread with the MPI_THREAD_MULTIPLE flag");
    }
    return threaded;
}

client_impl_t::client_impl_t(unsigned int id, const std::string &cfg_file) :
    cfg(cfg_file, false), rank(id) {
    if(cfg.is_sync() || check_threaded())
        start_main_loop(cfg, MPI_COMM_NULL);
    else
        launch_backend(cfg_file);
    queue = new comm_client_t<command_t>(rank);
    run_blocking(command_t(rank, command_t::INIT, 0, ""));
    DBG("VELOC initialized");
}

client_impl_t::client_impl_t(MPI_Comm c, const std::string &cfg_file) :
    cfg(cfg_file, false), comm(c) {
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &no_ranks);
    if (cfg.is_sync() || check_threaded()) {
	int provided;
        MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &local);
        MPI_Comm_rank(local, &provided);
        MPI_Comm_split(comm, provided == 0 ? 0 : MPI_UNDEFINED, rank, &backends);
        if (provided == 0)
            start_main_loop(cfg, backends);
        MPI_Barrier(local);
    } else
        launch_backend(cfg_file);
    aggregated = cfg.get_bool("aggregated", false);
    queue = new comm_client_t<command_t>(rank);
    run_blocking(command_t(rank, command_t::INIT, 0, ""));
    if (local != MPI_COMM_NULL)
	MPI_Barrier(local);
    DBG("VELOC initialized");
}

client_impl_t::~client_impl_t() {
    if (local != MPI_COMM_NULL) {
        MPI_Barrier(local);
        MPI_Comm_free(&local);
    }
    if (backends != MPI_COMM_NULL)
        MPI_Comm_free(&backends);
    delete queue;
    DBG("VELOC finalized");
}

bool client_impl_t::cleanup(const std::string &name) {
    parse_dir(cfg.get("scratch"), name, [](const std::string &fname, int, int) {
        remove(fname.c_str());
    });
    return true;
}

bool client_impl_t::checkpoint_wait() {
    if (checkpoint_in_progress) {
	ERROR("need to finalize local checkpoint first by calling checkpoint_end()");
	return false;
    }
    return queue->wait_completion() == VELOC_SUCCESS;
}

bool client_impl_t::checkpoint_finished() {
    if(cfg.is_sync())
        return true;
    if (checkpoint_in_progress) {
	ERROR("need to finalize local checkpoint first by calling checkpoint_end()");
	return false;
    }
    return queue->check_completion();
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
    if (aggregated) {
        long offset = 0, ckpt_size = file_size(current_ckpt.filename(cfg.get("scratch")));
	MPI_Exscan(&ckpt_size, &offset, 1, MPI_LONG, MPI_SUM, comm);
        DBG("Rank " << rank << ", offset = " << offset);
        if (rank == 0) {
            long offset_map[no_ranks + 1];
            offset_map[0] = no_ranks;
            MPI_Gather(&offset, 1, MPI_LONG, &offset_map[1], 1, MPI_LONG, 0, comm);
            if (!write_file(current_ckpt.agg_filename(cfg.get("meta")), (unsigned char *)offset_map, sizeof(long) * (no_ranks + 1)))
                return false;
        } else
            MPI_Gather(&offset, 1, MPI_LONG, NULL, no_ranks, MPI_LONG, 0, comm);
        current_ckpt.offset = offset;
    }
    checkpoint_in_progress = false;
    queue->enqueue(current_ckpt);
    auto it = observers.find(VELOC_OBSERVE_CKPT_END);
    if (it != observers.end())
	it->second(current_ckpt.name, current_ckpt.version);
    return cfg.is_sync() ? queue->wait_completion() == VELOC_SUCCESS : true;
}

int client_impl_t::run_blocking(const command_t &cmd) {
    queue->enqueue(cmd);
    return queue->wait_completion();
}

int client_impl_t::restart_test(const std::string &name, int needed_version, int target_rank) {
    if (!validate_name(name) || needed_version < 0) {
	ERROR("checkpoint name and/or version incorrect: name can only include [a-zA-Z0-9_] characters, version needs to be non-negative integer");
	return VELOC_FAILURE;
    }
    int version = run_blocking(command_t(check_rank(target_rank), command_t::TEST, needed_version, name.c_str()));
    DBG(name << ": latest version = " << version);
    if (comm != MPI_COMM_NULL && target_rank < 0) {
	int max_version;
        MPI_Allreduce(&version, &max_version, 1, MPI_INT, MPI_MAX, comm);
	return max_version;
    } else
	return version;
}

std::string client_impl_t::route_file(const std::string &original) {
    char abs_path[PATH_MAX + 1];
    if (original[0] != '/' && getcwd(abs_path, PATH_MAX) != NULL)
	current_ckpt.assign_path(std::string(abs_path) + "/" + original);
    else
	current_ckpt.assign_path(original);
    return current_ckpt.filename(cfg.get("scratch"));
}

bool client_impl_t::restart(const std::string &name, int version, int target_rank) {
    return restart_begin(name, version, target_rank)
        && recover_mem(VELOC_CKPT_ALL, {})
        && restart_end(true);
}

bool client_impl_t::restart_begin(const std::string &name, int version, int target_rank) {
    if (checkpoint_in_progress) {
	INFO("cannot restart while checkpoint in progress");
	return false;
    }
    if (!validate_name(name) || version < 0) {
	ERROR("checkpoint name and/or version incorrect: name can only include [a-zA-Z0-9_] characters, version needs to be non-negative integer");
	return false;
    }

    int result, end_result;
    current_ckpt = command_t(check_rank(target_rank), command_t::RESTART, version, name.c_str());
    result = run_blocking(current_ckpt);
    if (comm != MPI_COMM_NULL)
	MPI_Allreduce(&result, &end_result, 1, MPI_INT, MPI_LOR, comm);
    else
	end_result = result;
    if (end_result == VELOC_SUCCESS) {
        header_size = 0;
	return true;
    } else
	return false;
}

bool client_impl_t::read_current_header() {
    header_size = read_header(current_ckpt.filename(cfg.get("scratch")), region_info);
    return header_size != 0;
}

size_t client_impl_t::recover_size(int id) {
    if (header_size == 0)
        read_current_header();
    auto it = region_info.find(id);
    if (it == region_info.end())
	return 0;
    else
	return it->second;
}

bool client_impl_t::recover_mem(int mode, const std::set<int> &ids) {
    if (header_size == 0 && !read_current_header()) {
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
