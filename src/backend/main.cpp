#include <mpi.h>

#include "topology.hpp"
#include "common/ipc_queue.hpp"
#include "module_manager.hpp"

#define __DEBUG
#include "common/debug.hpp"

int main(int argc, char *argv[]) {
    int rank;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    topology_t top(MPI_COMM_WORLD, "antares-" + std::to_string(rank / 3));

    printf("Rank %d my right partner is %d\n", rank, top.get_partner(1));

    boost::interprocess::shared_memory_object::remove(veloc_ipc::shm_name);
    veloc_ipc::shm_queue_t<command_t> queue;
    module_manager_t modules;

    command_t c;
    while (true) {
	std::string id = queue.dequeue_any(c);
	modules.run(id, c);
    }

    MPI_Finalize();
    
    return 0;
}
