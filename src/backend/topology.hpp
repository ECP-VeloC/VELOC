#ifndef __TOPOLOGY_HPP
#define __TOPOLOGY_HPP

#include <boost/mpi.hpp>
#include <map>
#include <set>

class topology_t {
    boost::mpi::communicator mpi_comm;
    std::vector<int> shuffle, reorder;
    std::map<std::string, std::set<int> > domain_map;

public:
    topology_t(const MPI_Comm &comm, std::string failure_domain = "default");
    int get_partner(int distance);

private:
    bool save();
};

#endif
