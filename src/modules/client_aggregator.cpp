#include "client_aggregator.hpp"

//#define __DEBUG
#include "common/debug.hpp"

client_aggregator_t::client_aggregator_t(const agg_function_t &f, const single_function_t &g) :
    agg_function(f), single_function(g) { }

int client_aggregator_t::process_command(const command_t &c) {
    if (c.command == command_t::INIT) {
        std::unique_lock<std::mutex> cmds_lock(cmds_mutex);
        client_set.insert(c.unique_id);
        DBG("no of clients = " << client_set.size());
        cmds_lock.unlock();
        return single_function(c);
    } else if (c.command == command_t::TEST) {
        return single_function(c);
    } else if (c.command == command_t::CHECKPOINT || c.command == command_t::RESTART) {
        std::unique_lock<std::mutex> cmds_lock(cmds_mutex);
        cmds[c.command].push_back(c);
        if (cmds[c.command].size() == client_set.size()) {
	    res[c.command] = agg_function(cmds[c.command]);
	    cmds[c.command].clear();
            cmds_lock.unlock();
            cmds_cv.notify_all();
	} else {
            DBG("client " << c.unique_id << " waiting for " << c.command);
            while (cmds[c.command].size() > 0)
                cmds_cv.wait(cmds_lock);
            DBG("client " << c.unique_id << " notified for " << c.command);
        }
        return res[c.command];
    } else
	return VELOC_IGNORED;
}
