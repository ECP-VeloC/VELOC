#include "transfer_module.hpp"

#include "axl.h"
#include "common/file_util.hpp"

#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

transfer_module_t::transfer_module_t(const config_t &c) : cfg(c) {
    if (!cfg.storage()) {
        interval = -1;
        INFO("Persistent storage not specified, deactivating");
        return;
    }
    if (!cfg.get_optional("persistent_interval", interval)) {
	INFO("Persistence interval not specified, every checkpoint will be persisted");
	interval = 0;
    }
}

int transfer_module_t::process_command(const command_t &c) {
    if (interval < 0)
        return VELOC_IGNORED;

    std::string local = c.filename(cfg.get("scratch")), remote = c.stem();

    switch (c.command) {
    case command_t::INIT:
        ts_lock.lock();
        last_timestamp[c.unique_id] = std::chrono::system_clock::now() + std::chrono::seconds(interval);
        ts_lock.unlock();
        return VELOC_SUCCESS;

    case command_t::CHECKPOINT:
	if (interval > 0) {
            std::unique_lock<std::mutex> lock(ts_lock);
	    auto t = std::chrono::system_clock::now();
	    if (t < last_timestamp[c.unique_id])
		return VELOC_SUCCESS;
	    else
		last_timestamp[c.unique_id] = t + std::chrono::seconds(interval);
	}
	DBG("transfer local file " << local << " to " << remote);
        return cfg.storage()->flush(c) ? VELOC_SUCCESS : VELOC_FAILURE;

    case command_t::RESTART:
        DBG("checking local file: " << local);
        if (access(local.c_str(), R_OK) == 0)
	    return VELOC_SUCCESS;
	if (!cfg.storage()->exists(c)) {
	    ERROR("request to transfer remote file " << remote << " to " << local << " failed: source does not exist");
	    return VELOC_IGNORED;
	}
        DBG("transfer remote file " << remote << " to " << local);
	return cfg.storage()->restore(c) ? VELOC_SUCCESS : VELOC_IGNORED;

    default:
	return VELOC_IGNORED;
    }
}
