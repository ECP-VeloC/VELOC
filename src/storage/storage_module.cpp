#include "storage_module.hpp"

storage_module_t::storage_module_t(...) {
}

void storage_module_t::get_versions(const command_t &cmd, std::set<int> &result) {
}

bool storage_module_t::remove(const command_t &cmd) {
    return false;
}

bool storage_module_t::flush(const command_t &cmd) {
    return false;
}

bool storage_module_t::restore(const command_t &cmd) {
    return false;
}

bool storage_module_t::exists(const command_t &cmd) {
    return false;
}

storage_module_t::~storage_module_t() {
}
