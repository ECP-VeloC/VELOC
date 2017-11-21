#include <mpi.h>

#include "topology.hpp"

#define __DEBUG
#include "common/debug.hpp"

int main(int argc, char *argv[]) {
    int rank;
    MPI_Init(&argc, &argv);    

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    topology_t top(MPI_COMM_WORLD, "antares-" + std::to_string(rank / 3));

    printf("Rank %d my right partner is %d\n", rank, top.get_partner(1));

    MPI_Finalize();
    return 0;
}
