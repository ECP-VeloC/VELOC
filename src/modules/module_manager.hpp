#ifndef __MODULE_MANAGER_HPP
#define __MODULE_MANAGER_HPP

#include "common/command.hpp"
#include "common/status.hpp"
#include "common/config.hpp"
#include "modules/client_watchdog.hpp"
#include "modules/transfer_module.hpp"

#include <functional>
#include <boost/signals2.hpp>

class module_manager_t {    
    typedef std::function<int (const command_t &)> method_t;
    boost::signals2::signal<int (const command_t &)> sig;
    client_watchdog_t *watchdog = NULL;
    transfer_module_t *transfer = NULL;
    
public:
    module_manager_t();
    ~module_manager_t();
    void add_default_modules(const config_t &cfg);
    void add_module(const method_t &m) {
	sig.connect(m);
    }
    int notify_command(const command_t &c);
};

#endif // __MODULE_MANAGER_HPP
