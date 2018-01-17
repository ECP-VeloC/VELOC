#include "client_watchdog.hpp"

#include "common/status.hpp"

#define __DEBUG
#include "common/debug.hpp"

client_watchdog_t::client_watchdog_t(const config_t &c) : cfg(c) {
    auto optional_timeout = cfg.get_optional<int>("watchdog_interval");
    if (optional_timeout) {
	timeout = *optional_timeout;
	watchdog_thread = std::thread([this]() { timeout_check(); });
	watchdog_thread.detach();
    } else {
	timeout = 0;
	INFO("no watchdog_interval defined in config file, watchdog is disabled");
    }
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
	return VELOC_SUCCESS;
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
	return VELOC_SUCCESS;
    }
}
