#include "client_aggregator.hpp"

client_aggregator_t::client_aggregator_t(const agg_function_t &f) : agg_function(f) { }

int client_aggregator_t::process_command(const command_t &c) {
    switch (c.command) {
    case command_t::INIT:
	no_clients++;
	return VELOC_SUCCESS;
    default:
	cmds[c.command].push_back(c);
	int res = VELOC_SUCCESS;
	if (cmds[c.command].size() == no_clients) {
	    res = agg_function(c.command, cmds[c.command]);
	    cmds[c.command].clear();
	}
	return res;
    }
}
