#define __DEBUG
#include "common/debug.hpp"

#include <mpi.h>

#include "topology.hpp"

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    topology_t top(MPI_COMM_WORLD);

    INFO("Toplogy init successful, my right partner is: " << top.get_partner(1));

    MPI_Finalize();
    return 0;
}
