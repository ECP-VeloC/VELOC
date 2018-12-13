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
    case command_t::CHECKPOINT_BEGIN:	
	ckpt_count++;
	return VELOC_SUCCESS;
    case command_t::CHECKPOINT_CHUNK:
	ckpt_parts.push_back(c);
	return VELOC_SUCCESS;
    case command_t::CHECKPOINT_END:
	ckpt_count--;
	if (ckpt_count == 0) {
	    res = agg_function(ckpt_parts);
	    ckpt_parts.clear();
	}
	return res;
    case command_t::RESTART:
	restart_parts.push_back(c);
	if (restart_parts.size() == no_clients) {
	    res = agg_function(ckpt_parts);
	    restart_parts.clear();
	}
	return res;    
    default:
	return VELOC_SUCCESS;
    }
}
