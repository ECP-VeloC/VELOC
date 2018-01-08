#ifndef __POSIX_TRANSFER_HPP
#define __POSIX_TRANSFER_HPP

#include "common/command.hpp"
#include <boost/filesystem.hpp>

namespace bf = boost::filesystem;

class posix_transfer_t {
    bf::path scratch, persistent;
    
    int get_latest_version(const bf::path &p, const std::string &cname, int needed_id) {
	int ret = -1;
	for(auto& f : boost::make_iterator_range(bf::directory_iterator(p), {})) {
	    std::string fname = f.path().leaf().string();
	    int id, version;
	    if (bf::is_regular_file(f.path()) &&
		fname.compare(0, cname.length(), cname) == 0 &&
		sscanf(fname.substr(cname.length()).c_str(), "-%d-%d", &id, &version) == 2 &&
		id == needed_id) {	 
		if (version > ret)
		    ret = version;
	    }
	}
	return ret;
    }

public:
    posix_transfer_t(const bf::path &s, const bf::path &p) : scratch(s), persistent(p) { }
    
    void process_command(const command_t &c, const completion_t &completion) {
	auto remote = persistent / bf::path(c.ckpt_name).filename();
	switch (c.command) {
	case command_t::TEST:
	    DBG("obtain latest version for " << c.ckpt_name);
	    completion(std::max(get_latest_version(scratch, c.ckpt_name, c.unique_id),
				get_latest_version(persistent, c.ckpt_name, c.unique_id)));
	    return;
	    
	case command_t::CHECKPOINT:
	    DBG("transfer file " << c.ckpt_name << " to " << remote);
	    try {
		bf::copy_file(c.ckpt_name, remote, bf::copy_option::overwrite_if_exists);
	    } catch (bf::filesystem_error &e) {
		ERROR("cannot copy " <<  c.ckpt_name << " to " << remote << "; error message: " << e.what());
		completion(VELOC_FAILURE);
		return;
	    }
	    completion(VELOC_SUCCESS);
	    return;

	case command_t::RESTART:
	    DBG("transfer file " << remote << " to " << c.ckpt_name);
	    if (bf::exists(c.ckpt_name)) {
		INFO("request to transfer file " << remote << " to " << c.ckpt_name << " as destination already exists");
		completion(VELOC_SUCCESS);
		return;
	    }
	    if (!bf::exists(remote)) {
		ERROR("request to transfer file " << remote << " to " << c.ckpt_name << " ignored as source does not exist");
		completion(VELOC_FAILURE);
		return;
	    }
	    try {
		bf::copy_file(remote, c.ckpt_name, bf::copy_option::overwrite_if_exists);
	    } catch (bf::filesystem_error &e) {
		ERROR("cannot copy " <<  remote << " to " << c.ckpt_name << "; error message: " << e.what());
		completion(VELOC_FAILURE);
		return;
	    }
	    completion(VELOC_SUCCESS);
	    return;	   
	    
	default:
	    completion(VELOC_FAILURE);
	}
    }
};

#endif //__POSIX_TRANSFER_HPP
