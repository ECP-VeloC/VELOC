#include <system_error>
#include <fstream>

#include <boost/asio/ip/host_name.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "topology.hpp"

#define __DEBUG
#include "common/debug.hpp"

topology_t::topology_t(const MPI_Comm &comm, std::string failure_domain) :
    mpi_comm(comm, boost::mpi::comm_attach), shuffle(mpi_comm.size()), reorder(mpi_comm.size()) {
    std::string domain = failure_domain;
    std::vector<std::string> domain_list;

    if (domain == "default") {
	boost::system::error_code ec;
	domain = boost::asio::ip::host_name(ec);
	if (ec != boost::system::errc::success)
	    ERROR("rank " << mpi_comm.rank() << " can't get hostname");
    }
    boost::mpi::all_gather(mpi_comm, domain, domain_list);
    for (int i = 0; i < mpi_comm.size(); i++) 
	domain_map[domain_list[i]].insert(i);
    
    std::map<std::string, std::set<int>::const_iterator> domain_map_it;
    for (auto &el : domain_map)
	domain_map_it[el.first] = el.second.begin();
    int i = 0;
    while (i < mpi_comm.size()) {
	for (auto &host : domain_map) {
	    auto it = domain_map_it[host.first];
	    if (it != host.second.end()) {
		shuffle[*it] = i;
		reorder[i++] = *it;
		domain_map_it[host.first] = ++it;
	    }
	}
    }
    save();
}

int topology_t::get_partner(int distance) {
    return reorder[(shuffle[mpi_comm.rank()] + distance) % mpi_comm.size()];
}

bool topology_t::save() {
    std::ofstream ofs("/tmp/topology.out");
    boost::archive::binary_oarchive ta(ofs);
    ta << domain_map;

    return true;
}
