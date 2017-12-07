#ifndef __MODULE_MANAGER_HPP
#define __MODULE_MANAGER_HPP

#include "common/ipc_queue.hpp"

class module_manager_t {
public: 
    void run(const std::string &id, const command_t &c);
};

#endif // __MODULE_MANAGER_HPP
