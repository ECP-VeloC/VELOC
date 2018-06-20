#include "client_aggregator.hpp"

client_aggregator_t::client_aggregator_t(const agg_function_t &f, const single_function_t &g) :
    agg_function(f), single_function(g) { }

int client_aggregator_t::process_command(const command_t &c) {
    int res = VELOC_SUCCESS;
        switch (c.command) {
    case command_t::INIT:
	no_clients++;
	return single_function(c);
    case command_t::TEST:
	return single_function(c);
    case command_t::CHECKPOINT:
    case command_t::RESTART:
	cmds[c.command].push_back(c);
	if (cmds[c.command].size() == no_clients) {
	    res = agg_function(cmds[c.command]);
	    cmds[c.command].clear();
	}
	return res;
    default:
	return VELOC_SUCCESS;
    }
}
