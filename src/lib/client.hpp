#ifndef __CLIENT_HPP
#define __CLIENT_HPP

#include "common/config.hpp"
#include "common/ipc_queue.hpp"
#include "backend/module_manager.hpp"

#include <unordered_map>
#include <map>
#include <set>

#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

class veloc_client_t {    
    config_t cfg;

    typedef std::pair <void *, size_t> region_t;
    typedef std::map<int, region_t> regions_t;

    regions_t mem_regions;
    command_t current_ckpt;
    int rank;
    bool checkpoint_in_progress = false;

    veloc_ipc::shm_queue_t<command_t> *queue;
    module_manager_t *modules;

    command_t gen_ckpt_details(int cmd, const char *name, int version);
public:
    veloc_client_t(int r, const char *cfg_file);

    bool mem_protect(int id, void *ptr, size_t count, size_t base_size);
    bool mem_unprotect(int id);
    std::string route_file();
    
    bool checkpoint_begin(const char *name, int version);
    bool checkpoint_mem();
    bool checkpoint_end(bool success);

    int restart_test(const char *name);
    bool restart_begin(const char *name, int version);
    bool recover_mem(int mode, std::set<int> &ids);
    bool restart_end(bool success);

    ~veloc_client_t();
};

#endif
