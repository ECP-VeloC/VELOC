#ifndef __EC_MODULE_HPP
#define __EC_MODULE_HPP

#include "common/config.hpp"
#include "common/command.hpp"

#include <vector>
#include <chrono>
#include <deque>

#include <mpi.h>

class ec_module_t {
    config_t cfg;
    MPI_Comm comm, comm_domain;
    std::string fdomain;
    int scheme_id, interval, max_versions;
    std::chrono::system_clock::time_point last_timestamp;
    typedef std::map<std::string, std::deque<int> > checkpoint_history_t;
    checkpoint_history_t checkpoint_history;

public:
    ec_module_t(const config_t &c, MPI_Comm cm);
    ~ec_module_t();

    int process_commands(const std::vector<command_t> &cmds);
    int process_command(const command_t &cmd);
};

#endif //__EC_MODULE_HPP

