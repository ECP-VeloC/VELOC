#include "module_manager.hpp"

#define __DEBUG
#include "common/debug.hpp"

void module_manager_t::notify_command(const command_t &c, const completion_t &completion) {
    int result = VELOC_SUCCESS;
    sig(c, [&result](int status){ result = std::max(status, result); });
    completion(result);
}
