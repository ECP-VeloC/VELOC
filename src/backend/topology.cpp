#include <system_error>
#include <map>
#include <set>
#include <boost/asio/ip/host_name.hpp>

#include "topology.hpp"

#define __DEBUG
#include "common/debug.hpp"

topology_t::topology_t(const MPI_Comm &comm) : mpi_comm(comm, boost::mpi::comm_attach), shuffle(mpi_comm.size()) {
    boost::system::error_code ec;
    std::string host_name = boost::asio::ip::host_name(ec);
    std::vector<std::string> host_names;
    std::map<std::string, std::set<int> > host_to_id;
    
    if (ec != boost::system::errc::success) {
	ERROR("rank " << mpi_comm.rank() << " can't get hostname");
	host_name = "unknown";
    }
    boost::mpi::all_gather(mpi_comm, host_name, host_names);
    for (int i = 0; i < mpi_comm.size(); i++)
	host_to_id[host_names[i]].insert(i);

    int i = 0;
    while (i < mpi_comm.size()) {
	for (auto &host : host_to_id) {
	    auto it = host.second.begin();
	    if (it != host.second.end()) {
		shuffle[i++] = *it;
		host.second.erase(it);
	    }
	}
    }
}

int topology_t::get_partner(int distance) {
    return shuffle[(mpi_comm.rank() + distance) % mpi_comm.size()];
}
