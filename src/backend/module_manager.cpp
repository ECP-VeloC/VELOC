#include "module_manager.hpp"

#define __DEBUG
#include "common/debug.hpp"

int module_manager_t::notify_command(const command_t &c) {
    return *sig(c);
}
