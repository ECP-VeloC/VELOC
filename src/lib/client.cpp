#include "client.hpp"
#include "common/file_util.hpp"
#include "common/ckpt_util.hpp"
#include "backend/work_queue.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <future>
#include <queue>
#include <vector>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
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

bool client_impl_t::mem_protect(int id, void *ptr, size_t count, size_t base_size, const std::string &name) {
    return mem_regions[name].insert_or_assign(id, region_t(ptr, count * base_size)).second;
}

bool client_impl_t::mem_protect(int id, const serializer_t &s, const deserializer_t &d, const std::string &name) {
    return mem_regions[name].insert_or_assign(id, region_t(s, d)).second;
}

bool client_impl_t::mem_unprotect(int id, const std::string &name) {
    return mem_regions[name].erase(id) > 0;
}

void client_impl_t::mem_clear(const std::string &name) {
    mem_regions[name].clear();
}

bool client_impl_t::register_observer(int type, const observer_t &obs) {
    return observers.insert_or_assign(type, obs).second;
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
    // Drain the local (client-side) stage first; this also runs the deferred
    // backend enqueue registered in checkpoint_end().
    if (!current_group.wait_completion())
        return false;
    return queue->wait_completion() == VELOC_SUCCESS;
}

bool client_impl_t::checkpoint_finished() {
    if(cfg.is_sync())
        return true;
    if (checkpoint_in_progress) {
        ERROR("need to finalize local checkpoint first by calling checkpoint_end()");
        return false;
    }
    // local_durable becomes true only once the local stage completed and the
    // command was handed to the backend queue.
    return local_durable && queue->check_completion();
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
    regions_t ckpt_regions = get_current_ckpt_regions();
    if (mode == VELOC_CKPT_SOME) {
        for (auto it = ckpt_regions.begin(); it != ckpt_regions.end(); it++)
            if (ids.count(it->first) == 0)
                ckpt_regions.erase(it);
    } else if (mode == VELOC_CKPT_REST) {
        for (auto it = ids.begin(); it != ids.end(); it++)
            ckpt_regions.erase(*it);
    }
    if (ckpt_regions.size() == 0) {
        ERROR("empty selection of memory regions to checkpoint, please check protection and/or selective checkpointing primitives");
        return false;
    }

    std::string fname = current_ckpt.filename(cfg.get("scratch"));
    int fd = open(fname.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        ERROR("cannot open checkpoint file " << fname << ", reason: " << strerror(errno));
        return false;
    }

    auto &engine = transfer_engine_t::instance(cfg);
    current_group = engine.group();
    current_group.adopt_fd(fd);

    size_t regions_size = ckpt_regions.size();
    size_t header_size = sizeof(size_t) + regions_size * (sizeof(int) + sizeof(size_t));

    // Determine each region's payload and size. Serialized regions are captured
    // into group-owned buffers now, so their size is known up front and their
    // lifetime extends past this call (writes complete in the background).
    std::vector<int> ids_ord;
    std::vector<size_t> sizes;
    std::vector<const void *> srcs;
    ids_ord.reserve(regions_size);
    sizes.reserve(regions_size);
    srcs.reserve(regions_size);
    for (auto &e : ckpt_regions) {
        region_t &info = e.second;
        ids_ord.push_back(e.first);
        if (info.ptr != NULL) {
            sizes.push_back(info.size);
            srcs.push_back(info.ptr);
        } else {
            std::ostringstream oss(std::ios::out | std::ios::binary);
            info.s(oss);
            std::string s = oss.str();
            void *buf = current_group.alloc(s.size());
            memcpy(buf, s.data(), s.size());
            info.size = s.size();
            sizes.push_back(s.size());
            srcs.push_back(buf);
        }
    }

    // Build the header (region count + per-region id/size) with the same on-disk
    // layout as before, into a group-owned buffer.
    void *hbuf = current_group.alloc(header_size);
    char *hp = (char *)hbuf;
    memcpy(hp, &regions_size, sizeof(size_t));
    hp += sizeof(size_t);
    for (size_t k = 0; k < regions_size; k++) {
        memcpy(hp, &ids_ord[k], sizeof(int));
        hp += sizeof(int);
        memcpy(hp, &sizes[k], sizeof(size_t));
        hp += sizeof(size_t);
    }

    // Submit header then each region at its computed offset. The engine detects
    // device vs host pointers and inserts the GPU->host bounce transparently.
    current_ckpt_size = header_size;
    current_group.submit(mem(hbuf), file(fd, 0), header_size);
    size_t off = header_size;
    for (size_t k = 0; k < regions_size; k++) {
        current_group.submit(mem(const_cast<void *>(srcs[k])), file(fd, off), sizes[k]);
        off += sizes[k];
        current_ckpt_size += sizes[k];
    }

    // Block only until the application's memory has been captured (device D2H
    // done, host writes done); staging->file writes continue in the background.
    return current_group.wait_sources();
}

bool client_impl_t::checkpoint_end(bool /*success*/) {
    if (aggregated) {
        // Use the logical checkpoint size (known from the layout) instead of
        // stat()-ing the file, which may still be written in the background.
        long offset = 0, ckpt_size = (long)current_ckpt_size;
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

    auto notify_observer = [this]() {
        auto it = observers.find(VELOC_OBSERVE_CKPT_END);
        if (it != observers.end())
            it->second(current_ckpt.name, current_ckpt.version);
    };

    if (cfg.is_sync()) {
        bool ok = current_group.wait_completion();
        queue->enqueue(current_ckpt);
        notify_observer();
        return ok && queue->wait_completion() == VELOC_SUCCESS;
    }

    // Async: hand the checkpoint to the backend the moment the local file is
    // durable (on the engine progress thread), preserving compute/flush overlap.
    local_durable = false;
    command_t cmd = current_ckpt;
    current_group.on_completion([this, cmd]() {
        queue->enqueue(cmd);
        local_durable = true;
    });
    notify_observer();
    return true;
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
    regions_t &ckpt_regions = get_current_ckpt_regions();

    std::string fname = current_ckpt.filename(cfg.get("scratch"));
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd == -1) {
        ERROR("cannot open checkpoint file " << fname << ", reason: " << strerror(errno));
        return false;
    }

    auto &engine = transfer_engine_t::instance(cfg);
    xfer_group_t g = engine.group();
    g.adopt_fd(fd);

    // Serialized regions are read into group-owned buffers first, then
    // deserialized from host memory once all reads complete.
    struct deser_t { void *buf; size_t size; region_t *info; int id; };
    std::vector<deser_t> deser;

    size_t off = header_size;
    for (auto &e : region_info) {
        bool found = ids.find(e.first) != ids.end();
        if ((mode == VELOC_RECOVER_SOME && !found) || (mode == VELOC_RECOVER_REST && found)) {
            off += e.second;
            continue;
        }
        auto it = ckpt_regions.find(e.first);
        if (it == ckpt_regions.end()) {
            ERROR("no protected memory region defined for id " << e.first);
            return false;
        }
        region_t &info = it->second;
        if (info.ptr != NULL) { // direct read of raw data (host or device, engine decides)
            if (info.size < e.second) {
                ERROR("protected memory region " << e.first << " is too small ("
                      << info.size << ") to hold required size ("
                      << e.second << ")");
                return false;
            }
            g.submit(file(fd, off), mem(info.ptr), e.second);
        } else { // read raw bytes into a host buffer, deserialize after completion
            void *buf = g.alloc(e.second);
            g.submit(file(fd, off), mem(buf), e.second);
            deser.push_back({buf, e.second, &info, e.first});
        }
        off += e.second;
    }

    if (!g.wait_completion()) {
        ERROR("cannot read checkpoint file " << current_ckpt);
        return false;
    }

    for (auto &d : deser) {
        std::string s((const char *)d.buf, d.size);
        std::istringstream iss(s, std::ios::in | std::ios::binary);
        if (!d.info->d(iss)) {
            ERROR("protected data structure " << d.id << " could not be deserialized");
            return false;
        }
    }
    return true;
}

bool client_impl_t::restart_end(bool /*success*/) {
    return true;
}
