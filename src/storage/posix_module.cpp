#include "posix_module.hpp"
#include "common/file_util.hpp"

#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

posix_module_t::posix_module_t(const std::string &s, const std::string &p) : scratch(s), persistent(p) {
    if (!check_dir(persistent))
	FATAL("persistent directory " << persistent << " inaccessible!");
}

void posix_module_t::get_versions(const command_t &cmd, std::set<int> &result) {
    parse_dir(persistent, cmd.name,
              [&](const std::string &f, const std::string &sid, const std::string &sv) {
                  if (std::stoi(sid) == cmd.unique_id)
                      result.insert(std::stoi(sv));
              });
}

bool posix_module_t::remove(const command_t &cmd) {
    bool success = unlink(cmd.filename(persistent).c_str()) == 0;
    if (!success)
        ERROR("failed to remove " << cmd.filename(persistent) << ", error = " << std::strerror(errno));
    return success;
}

bool posix_module_t::flush(const command_t &cmd) {
    // memory-based API
    if (cmd.original[0] == 0)
        return posix_transfer_file(cmd.filename(scratch), cmd.filename(persistent));
    // file-based API
    if (!posix_transfer_file(cmd.filename(scratch), cmd.original))
        return false;
    unlink(cmd.filename(persistent).c_str());
    if (symlink(cmd.original, cmd.filename(persistent).c_str())) {
        ERROR("cannot create symlink " << cmd.filename(persistent) << " pointing at " << cmd.original << ", error: " << std::strerror(errno));
        return false;
    }
    return true;
}

bool posix_module_t::restore(const command_t &cmd) {
    return posix_transfer_file(cmd.filename(persistent), cmd.filename(scratch));
}

bool posix_module_t::exists(const command_t &cmd) {
    return access(cmd.filename(persistent).c_str(), R_OK) == 0;
}

posix_module_t::~posix_module_t() {
}
