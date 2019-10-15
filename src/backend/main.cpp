#include "common/config.hpp"
#include "common/command.hpp"
#include "common/ipc_queue.hpp"

#include "modules/module_manager.hpp"

#include <queue>
#include <future>

#define __DEBUG
#include "common/debug.hpp"

const unsigned int DEFAULT_PARALLELISM = 64;

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
	veloc_ipc::cleanup();
	std::cout << "Usage: " << argv[0] << " <veloc_config> [--disable-ec]" << std::endl;
	return 1;
    }

    config_t cfg(argv[1]);
    if (cfg.is_sync()) {
	ERROR("configuration requests sync mode, backend is not needed");
	return 3;
    }
    if (argc == 3 && std::string(argv[2]) == "--disable-ec")
	INFO("EC module not available in barebone version, disabled by default");

    int max_parallelism;
    if (!cfg.get_optional("max_parallelism", max_parallelism))
        max_parallelism = DEFAULT_PARALLELISM;
    INFO("Max number of client requests processed in parallel: " << max_parallelism);

    veloc_ipc::cleanup();
    veloc_ipc::shm_queue_t<command_t> command_queue(NULL);
    module_manager_t modules;
    modules.add_default_modules(cfg);

    std::queue<std::future<void> > work_queue;
    command_t c;
    while (true) {
	auto f = command_queue.dequeue_any(c);
	work_queue.push(std::async(std::launch::async, [=, &modules] {
		    f(modules.notify_command(c));
		}));
	if (work_queue.size() > (unsigned int)max_parallelism) {
	    work_queue.front().wait();
	    work_queue.pop();
	}
    }

    return 0;
}
