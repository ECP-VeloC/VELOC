#ifndef __CLIENT_AGGREGATOR_HPP
#define __CLIENT_AGGREGATOR_HPP

#include "common/command.hpp"
#include "common/status.hpp"

#include <functional>
#include <vector>
#include <map>

class client_aggregator_t {
    unsigned int no_clients;
    typedef std::function<int (const std::vector<command_t> &)> agg_function_t;
    agg_function_t agg_function;
    std::map<int, std::vector<command_t> > cmds;
public:
    client_aggregator_t(const agg_function_t &f);
    int process_command(const command_t &c);
};

#endif //__AGGREGATOR_MODULE_HPP
