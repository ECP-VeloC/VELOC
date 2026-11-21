#include "versioning_module.hpp"
#include "common/file_util.hpp"

#include <unistd.h>
#include <vector>

//#define __DEBUG
#include "common/debug.hpp"

static void scratch_version_set(const std::string &p, const char *cname, int req_id, std::set<int> &result) {
    parse_dir(p, cname,
              [&](const std::string &, int id, int v) {
                  if (id == command_t::ID_EC || id == req_id)
                      result.insert(v);
              });
}

versioning_module_t::versioning_module_t(const config_t &c) : cfg(c) {
    if (cfg.storage() && !cfg.get_optional("max_versions", max_versions)) {
        max_versions = 0;
        INFO("persisting last " << max_versions << " checkpoints to " << cfg.get("persistent") << " (0 means all), change using 'max_versions'");
    }

    if (!cfg.get_optional("scratch_versions", scratch_versions)) {
        scratch_versions = 0;
        INFO("caching last " << scratch_versions << " checkpoints to " << cfg.get("scratch") << " (0 means all), change using 'scratch_versions'");
    }
    cfg.get_optional("meta", meta);
}

int versioning_module_t::process_command(const command_t &c) {
    std::unique_lock<std::mutex> lock(history_lock);
    auto &ph = persistent_history[c.name][c.unique_id];
    auto &sh = scratch_history[c.name][c.unique_id];
    std::set<int, std::greater<int> > versions;

    switch (c.command) {
    case command_t::TEST:
        ph.clear();
        if (cfg.storage())
            cfg.storage()->get_versions(c, ph);
        sh.clear();
        scratch_version_set(cfg.get("scratch"), c.name, c.unique_id, sh);
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

    case command_t::CHECKPOINT: {
        // Pick the old versions under the lock, then release it before deleting.
        // One module instance serves all ranks of a node (sync: local rank 0's
        // main loop; async: veloc-backend). Holding the lock during unlink made
        // the ranks delete one after another.
        std::vector<int> old_p, old_s;
        if (cfg.storage() && max_versions > 0) {
            ph.insert(c.version);
            auto it = ph.begin();
            while (it != ph.end() && ph.size() > (unsigned int)max_versions) {
                old_p.push_back(*it);
                it = ph.erase(it);
            }
        }
        if (scratch_versions > 0) {
            sh.insert(c.version);
            auto it = sh.begin();
            while (it != sh.end() && sh.size() > (unsigned int)scratch_versions) {
                old_s.push_back(*it);
                it = sh.erase(it);
            }
        }
        lock.unlock();

        // delete old versions on persistent mount point
        if (!old_p.empty()) {
            TIMER_START(del_persistent);
            for (int v : old_p) {
                command_t old = c;
                old.version = v;
                cfg.storage()->remove(old);
                if (!meta.empty())
                    unlink(old.meta_filename(meta).c_str());
            }
            TIMER_STOP(del_persistent, "deleted " << old_p.size() << " old version(s) of " << c.name << " id " << c.unique_id << " on persistent (new v" << c.version << ")");
        }
        // delete old versions on scratch mount point: only this rank's files
        // (and shared EC files), as in scratch_version_set
        if (!old_s.empty()) {
            TIMER_START(del_scratch);
            for (int v : old_s)
                parse_dir(cfg.get("scratch"), c.name,
                          [&](const std::string &f, int id, int fv) {
                              if (fv == v && (id == c.unique_id || id == command_t::ID_EC))
                                  remove(f.c_str());
                          });
            TIMER_STOP(del_scratch, "deleted " << old_s.size() << " old version(s) of " << c.name << " id " << c.unique_id << " on scratch (new v" << c.version << ")");
        }
        return VELOC_SUCCESS;
    }

    case command_t::RESTART:
        if (cfg.storage() && cfg.storage()->exists(c))
            ph.insert(c.version);
        if (access(c.filename(cfg.get("scratch")).c_str(), R_OK) == 0)
            sh.insert(c.version);
        return VELOC_IGNORED;

    default:
        return VELOC_IGNORED;
    }
}
