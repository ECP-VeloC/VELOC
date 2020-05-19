#ifndef __CLIENT_AGGREGATOR_HPP
#define __CLIENT_AGGREGATOR_HPP

#include "common/command.hpp"
#include "common/status.hpp"

#include <functional>
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <condition_variable>

class client_aggregator_t {

    typedef std::function<int (const std::vector<command_t> &)> agg_function_t;
    typedef std::function<int (const command_t &)> single_function_t;
    agg_function_t agg_function;
    single_function_t single_function;

    std::mutex cmds_mutex;
    std::condition_variable cmds_cv;
    std::map<int, std::vector<command_t> > cmds;
    std::set<int> client_set;
    std::map<int, int> res;
public:
    client_aggregator_t(const agg_function_t &f, const single_function_t &g);
    int process_command(const command_t &c);
};

#endif //__AGGREGATOR_MODULE_HPP
