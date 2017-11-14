#ifndef __TOPOLOGY_HPP
#define __TOPOLOGY_HPP

#include <boost/mpi.hpp>

class topology_t {
    boost::mpi::communicator mpi_comm;
    std::vector<int> shuffle;

public:
    topology_t(const MPI_Comm &comm);
    int get_partner(int distance);
};

#endif
