#ifndef __MODULE_MANAGER_HPP
#define __MODULE_MANAGER_HPP

#include "common/command.hpp"
#include "common/status.hpp"

#include <functional>
#include <boost/signals2.hpp>

class module_manager_t {    
    typedef std::function<int (const command_t &)> method_t;
    boost::signals2::signal<int (const command_t &)> sig;

public:
    void add_module(const method_t &m) {
	sig.connect(m);
    }
    int notify_command(const command_t &c);    
};

#endif // __MODULE_MANAGER_HPP
