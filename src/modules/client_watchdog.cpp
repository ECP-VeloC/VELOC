#include "client_watchdog.hpp"

#include "common/status.hpp"

//#define __DEBUG
#include "common/debug.hpp"

client_watchdog_t::client_watchdog_t(const config_t &c) : cfg(c) {
    if(cfg.get_optional("watchdog_interval", timeout)) {
	watchdog_thread = std::thread([this]() { timeout_check(); });
	watchdog_thread.detach();
	INFO("watchdog enabled, checking liveness every " << timeout << " seconds");
    } else
	timeout = 0;
}

void client_watchdog_t::timeout_check() {
    while (true) {
	auto t = std::chrono::system_clock::now();
	for (auto &e : client_map)
	    if (t > e.second) {
		ERROR("client " << e.first << " triggered a watchdog timeout");
		// TODO: kill client and initiate restart
	    }
	std::this_thread::sleep_for(std::chrono::seconds(timeout));
    }
}

int client_watchdog_t::process_command(const command_t &c) {
    if (timeout == 0)
	return VELOC_IGNORED;

    switch(c.command) {
    case command_t::INIT:
	if (client_map.find(c.unique_id) == client_map.end())
	    INFO("new client " << c.unique_id << " registered");
	client_map[c.unique_id] = std::chrono::system_clock::now() + std::chrono::seconds(timeout);
	DBG("watchdog init, next check in " << timeout << " seconds");
        return VELOC_SUCCESS;

    case command_t::CHECKPOINT:
	if (client_map.find(c.unique_id) == client_map.end()) {
	    ERROR("unknown client " << c.unique_id << " issued checkpoint request");
	    return VELOC_FAILURE;
	}
	client_map[c.unique_id] = std::chrono::system_clock::now() + std::chrono::seconds(timeout);
	DBG("watchdog ping, next check in " << timeout << " seconds");
	return VELOC_SUCCESS;

    default:
	return VELOC_IGNORED;
    }
}
