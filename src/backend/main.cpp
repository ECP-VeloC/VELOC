#include <mpi.h>

#include "common/config.hpp"
#include "common/command.hpp"
#include "common/ipc_queue.hpp"

#include "topology.hpp"

#include "modules/module_manager.hpp"

#define __DEBUG
#include "common/debug.hpp"

int main(int argc, char *argv[]) {
    if (argc != 2) {
	veloc_ipc::cleanup();
	std::cout << "Usage: " << argv[0] << " <veloc_config>" << std::endl;
	return 1;
    }

    config_t cfg;
    if (!cfg.init(argv[1]))
	return 2;
    if (cfg.is_sync()) {
	ERROR("configuration requests sync mode, backend is not needed");
	return 3;
    }
    
    int rank;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    topology_t top(MPI_COMM_WORLD, "antares-" + std::to_string(rank / 3));

    printf("Rank %d my right partner is %d\n", rank, top.get_partner(1));

    veloc_ipc::cleanup();
    veloc_ipc::shm_queue_t<command_t> queue(NULL);
    module_manager_t modules;
    modules.add_default_modules(cfg);
    
    command_t c;
    while (true) {
	veloc_ipc::completion_t completion = queue.dequeue_any(c);
	completion(modules.notify_command(c));
    }
    MPI_Finalize();
    
    return 0;
}
