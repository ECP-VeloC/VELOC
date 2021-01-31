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
    config_t cfg;
    MPI_Comm comm;
    bool collective, ec_active;
    int rank;

    command_t current_ckpt;
    bool checkpoint_in_progress = false;

    std::map<int, size_t> region_info;
    size_t header_size = 0;

    comm_client_t<command_t> *queue = NULL;
    module_manager_t *modules = NULL;

    int run_blocking(const command_t &cmd);
    bool read_header();

public:
    client_impl_t(unsigned int id, const std::string &cfg_file);
    client_impl_t(MPI_Comm comm, const std::string &cfg_file);

    virtual std::string route_file(const std::string &original);

    virtual bool checkpoint(const std::string &name, int version);
    virtual bool checkpoint_begin(const std::string &name, int version);
    virtual bool checkpoint_mem(int mode, const std::set<int> &ids);
    virtual bool checkpoint_end(bool success);
    virtual bool checkpoint_wait();

    virtual int restart_test(const std::string &name, int version);
    virtual bool restart(const std::string &name, int version);
    virtual bool restart_begin(const std::string &name, int version);
    virtual size_t recover_size(int id);
    virtual bool recover_mem(int mode, const std::set<int> &ids);
    virtual bool restart_end(bool success);

    virtual ~client_impl_t();
};

#endif
