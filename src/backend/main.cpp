#include "common/config.hpp"
#include "common/command.hpp"
#include "common/ipc_queue.hpp"

#include "modules/module_manager.hpp"

#include <queue>
#include <future>

#define __DEBUG
#include "common/debug.hpp"

const unsigned int MAX_PARALLELISM = 64;

int main(int argc, char *argv[]) {
    bool ec_active = true;
    
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
    if (argc == 3 && std::string(argv[2]) == "--disable-ec") {
	INFO("EC module disabled by commmand line switch");
	ec_active = false;
    }

    if (ec_active) {
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	DBG("Active backend rank = " << rank);
    }

    veloc_ipc::cleanup();
    veloc_ipc::shm_queue_t<command_t> command_queue(NULL);
    module_manager_t modules;
    modules.add_default_modules(cfg, MPI_COMM_WORLD, ec_active);

    std::queue<std::future<void> > work_queue;
    command_t c;
    while (true) {
	auto f = command_queue.dequeue_any(c);
	work_queue.push(std::async(std::launch::async, [=, &modules] {
		    f(modules.notify_command(c));
		}));
	if (work_queue.size() > MAX_PARALLELISM) {
	    work_queue.front().wait();
	    work_queue.pop();
	}
    }
    
    if (ec_active) {
	MPI_Finalize();
    }

    return 0;
}
