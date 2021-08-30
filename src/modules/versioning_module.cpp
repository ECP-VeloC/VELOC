#include "versioning_module.hpp"
#include "common/file_util.hpp"

#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

static void get_version_set(const std::string &p, const char *cname, int id, std::set<int> &result) {
    parse_dir(p, cname,
              [&](const std::string &f, const std::string &sid, const std::string &sv) {
                  if (sid == "ec" || std::stoi(sid) == id)
                      result.insert(std::stoi(sv));
              });
}

versioning_module_t::versioning_module_t(const config_t &c) : cfg(c) {
    if (!cfg.get_optional("max_versions", max_versions)) {
	max_versions = 0;
        INFO("persisting last " << max_versions << " checkpoints to " << cfg.get("persistent") << " (0 means all), change using 'max_versions'");
    }

    if (!cfg.get_optional("scratch_versions", scratch_versions)) {
        scratch_versions = 0;
        INFO("caching last " << scratch_versions << " checkpoints to " << cfg.get("scratch") << " (0 means all), change using 'scratch_versions'");
    }
}

int versioning_module_t::process_command(const command_t &c) {
    std::unique_lock<std::mutex> lock(history_lock);
    auto &ph = persistent_history[c.name][c.unique_id];
    auto &sh = scratch_history[c.name][c.unique_id];
    std::set<int, std::greater<int> > versions;

    switch (c.command) {
    case command_t::TEST:
        ph.clear();
        get_version_set(cfg.get("persistent"), c.name, c.unique_id, ph);
        sh.clear();
        get_version_set(cfg.get("scratch"), c.name, c.unique_id, sh);
        std::set_union(ph.begin(), ph.end(), sh.begin(), sh.end(),
                       std::inserter(versions, versions.begin()));

        if (versions.size() == 0)
            return VELOC_IGNORED;
        if (c.version == 0)
            return *versions.begin();
        else {
            auto it = versions.lower_bound(c.version);
            return it != versions.end() ? *it : VELOC_IGNORED;
        }

    case command_t::CHECKPOINT:
        // delete old versions on persistent mount point
        if (max_versions > 0) {
            ph.insert(c.version);
            auto it = ph.begin();
            while (it != ph.end() && ph.size() > (unsigned int)max_versions) {
                remove(c.filename(cfg.get("persistent"), *it).c_str());
                it = ph.erase(it);
            }
        }
        // delete old versions on scratch mount point
        if (scratch_versions > 0) {
            sh.insert(c.version);
            auto it = sh.begin();
            while (it != sh.end() && sh.size() > (unsigned int)scratch_versions) {
                parse_dir(cfg.get("scratch"), c.name,
                          [&](const std::string &f, const std::string &, const std::string &sv) {
                              if (std::stoi(sv) == *it)
                                  remove(f.c_str());
                          });
                it = sh.erase(it);
            }
        }
        return VELOC_SUCCESS;

    case command_t::RESTART:
        if (access(c.filename(cfg.get("persistent")).c_str(), R_OK) == 0)
            ph.insert(c.version);
        if (access(c.filename(cfg.get("scratch")).c_str(), R_OK) == 0)
            sh.insert(c.version);
        return VELOC_IGNORED;

    default:
        return VELOC_IGNORED;
    }
}
