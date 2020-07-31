#ifndef __MODULE_MANAGER_HPP
#define __MODULE_MANAGER_HPP

#include "common/command.hpp"
#include "common/status.hpp"
#include "common/config.hpp"
#include "modules/client_watchdog.hpp"
#include "modules/client_aggregator.hpp"
#include "modules/ec_module.hpp"
#include "modules/transfer_module.hpp"
#include "modules/chksum_module.hpp"
#include "modules/versioning_module.hpp"

#include <functional>
#include <vector>

#include <mpi.h>

class module_manager_t {
    typedef std::function<int (const command_t &)> method_t;
    std::vector<method_t> sig;
    client_watchdog_t *watchdog = NULL;
    transfer_module_t *transfer = NULL;
    client_aggregator_t *ec_agg = NULL;
    ec_module_t *redset = NULL;
    chksum_module_t *chksum = NULL;
    versioning_module_t *versioning = NULL;

public:
    module_manager_t();
    ~module_manager_t();
    void add_default_modules(const config_t &cfg);
    void add_default_modules(const config_t &cfg, MPI_Comm comm, bool ec_active);
    void add_module(const method_t &m) {
	sig.push_back(m);
    }
    int notify_command(const command_t &c);
};

#endif // __MODULE_MANAGER_HPP
