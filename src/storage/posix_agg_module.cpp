#include "posix_agg_module.hpp"
#include "common/file_util.hpp"

#include <unistd.h>
#include <fcntl.h>

//#define __DEBUG
#include "common/debug.hpp"

posix_agg_module_t::posix_agg_module_t(const std::string &s, const std::string &p, const std::string &m) : posix_module_t(s, p), meta(m) {
    if (!check_dir(meta))
        FATAL("metadata directory " << meta << " inaccessible!");
}

void posix_agg_module_t::get_versions(const command_t &cmd, std::set<int> &result) {
    parse_dir(persistent, cmd.name,
              [&](const std::string &f, int id, int v) {
                  if (id == command_t::ID_AGG)
                      result.insert(v);
              });
}

bool posix_agg_module_t::flush(const command_t &cmd) {
    // aggregated mode supported for memory-based API only
    return posix_transfer_file(cmd.filename(scratch), cmd.agg_filename(persistent), 0, cmd.offset, file_size(cmd.filename(scratch)));
}

bool posix_agg_module_t::remove(const command_t &cmd) {
    return unlink(cmd.agg_filename(persistent).c_str());
}

bool posix_agg_module_t::restore(const command_t &cmd) {
    // read header to find out rank offset
    std::string meta_file = cmd.agg_filename(meta);
    int fi = open(meta_file.c_str(), O_RDONLY);
    if (fi == -1) {
	ERROR("cannot open aggregated header " << meta_file << "; error = " << std::strerror(errno));
	return false;
    }
    long offset[2], num_ranks;
    ssize_t res = pread(fi, &num_ranks, sizeof(long), 0);
    if (res == -1) {
        ERROR("cannot read number of ranks from aggregated header " << meta_file << "; error = " << std::strerror(errno));
        close(fi);
        return false;
    }
    if (cmd.unique_id == num_ranks - 1) {
        res = pread(fi, &offset[0], sizeof(long), (cmd.unique_id + 1) * sizeof(long));
        offset[1] = std::numeric_limits<long>::max();
    } else {
        res = pread(fi, &offset[0], 2 * sizeof(long), (cmd.unique_id + 1) * sizeof(long));
        offset[1] -= offset[0];
    }
    close(fi);
    if (res == -1) {
        ERROR("cannot read offset from aggregated header " << meta_file << "; error = " << std::strerror(errno));
        return false;
    }
    DBG("rank " << cmd.unique_id << ", reading from offset " << offset[0] << ", size = " << offset[1]);
    // reconstruct local file from starting from rank offset
    return posix_transfer_file(cmd.agg_filename(persistent), cmd.filename(scratch), offset[0], 0, offset[1]);
}

bool posix_agg_module_t::exists(const command_t &cmd) {
    return access(cmd.agg_filename(persistent).c_str(), R_OK) == 0;
}

posix_agg_module_t::~posix_agg_module_t() {
}
