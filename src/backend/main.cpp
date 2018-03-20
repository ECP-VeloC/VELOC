#include <mpi.h>

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
    if (argc != 2) {
	veloc_ipc::cleanup();
	std::cout << "Usage: " << argv[0] << " <veloc_config>" << std::endl;
	return 1;
    }

    config_t cfg(argv[1]);
    if (cfg.is_sync()) {
	ERROR("configuration requests sync mode, backend is not needed");
	return 3;
    }
    
    int rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    INFO("MPI rank = " << rank);

    veloc_ipc::cleanup();
    veloc_ipc::shm_queue_t<command_t> command_queue(NULL);
    module_manager_t modules;
    modules.add_default_modules(cfg);

    std::queue<std::future<void> > work_queue;
    while (true) {
	work_queue.push(std::async(std::launch::async, [&modules,&command_queue] {
		    command_t c;
		    command_queue.dequeue_any(c)(modules.notify_command(c));
		}));
	if (work_queue.size() > MAX_PARALLELISM) {
	    work_queue.front().wait();
	    work_queue.pop();
	}
    }
    MPI_Finalize();

    return 0;
}
