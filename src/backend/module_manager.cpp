#include "module_manager.hpp"

#define __DEBUG
#include "common/debug.hpp"

void module_manager_t::run(const std::string &id, const command_t &c) {
    DBG("Got command from process " << id << ": " << c);
}
