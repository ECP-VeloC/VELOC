#ifndef __CLIENT_HPP
#define __CLIENT_HPP

#include "include/veloc.hpp"

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/comm_queue.hpp"
#include "modules/module_manager.hpp"

#include <unordered_map>
#include <map>

class client_impl_t : public veloc::client_t {
    struct region_t {
        void *ptr;
        size_t size;
        serializer_t s;
        deserializer_t d;
        region_t(void *p, size_t s) : ptr(p), size(s) { }
        region_t(const serializer_t &_s, const deserializer_t &_d) : ptr(NULL), s(_s), d(_d) { }
    };
    using regions_t = std::map<int, region_t>;
    using regions_map_t = std::unordered_map<std::string, regions_t>;
    using observers_t = std::map<int, observer_t>;

    config_t cfg;
    MPI_Comm comm = MPI_COMM_NULL, local = MPI_COMM_NULL, backends = MPI_COMM_NULL;
    int rank, no_ranks;

    regions_map_t mem_regions;
    observers_t observers;

    command_t current_ckpt;
    bool checkpoint_in_progress = false, aggregated = false;

    std::map<int, size_t> region_info;
    size_t header_size = 0;
    comm_client_t<command_t> *queue = NULL;

    bool check_threaded();
    int run_blocking(const command_t &cmd);
    bool read_current_header();

    int check_rank(int target_rank) {
	return target_rank < 0 ? rank : target_rank;
    }

    regions_t &get_current_ckpt_regions() {
	regions_t &ckpt_regions = mem_regions[current_ckpt.name];
	if (ckpt_regions.empty())
	    ckpt_regions = mem_regions[""];
	return ckpt_regions;
    }

public:
    client_impl_t(unsigned int id, const std::string &cfg_file);
    client_impl_t(MPI_Comm comm, const std::string &cfg_file);

    virtual bool mem_protect(int id, void *ptr, size_t count, size_t base_size, const std::string &name);
    virtual bool mem_protect(int id, const serializer_t &s, const deserializer_t &d, const std::string &name);
    virtual bool mem_unprotect(int id, const std::string &name);
    virtual void mem_clear(const std::string &name);
    virtual bool register_observer(int type, const observer_t &obs);

    virtual std::string route_file(const std::string &original);
    virtual bool cleanup(const std::string &name);

    virtual bool checkpoint(const std::string &name, int version);
    virtual bool checkpoint_begin(const std::string &name, int version);
    virtual bool checkpoint_mem(int mode, const std::set<int> &ids);
    virtual bool checkpoint_end(bool success);
    virtual bool checkpoint_wait();
    virtual bool checkpoint_finished();

    virtual int restart_test(const std::string &name, int version, int target_rank);
    virtual bool restart(const std::string &name, int version, int target_rank);
    virtual bool restart_begin(const std::string &name, int version, int target_rank);
    virtual size_t recover_size(int id);
    virtual bool recover_mem(int mode, const std::set<int> &ids);
    virtual bool restart_end(bool success);

    virtual ~client_impl_t();
};

#endif
