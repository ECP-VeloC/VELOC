#ifndef __CACHE_STRATEGY_HPP
#define __CACHE_STRATEGY_HPP

#include <atomic>
#include <mutex>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/math/interpolators/cubic_b_spline.hpp>
#include <boost/circular_buffer.hpp>

#define __DEBUG
#include "common/debug.hpp"

using namespace boost::interprocess;

class cache_strategy_t {
    const unsigned int SEGMENT_SIZE = 1 << 16;
    const std::vector<double> ssd_y { 2523, 3179, 3382, 9694, 41383, 55157, 69320, 88404, 95403, 99116, 109274 };
    
    struct monitor_t {
	const unsigned int CACHE_SLOTS = 128, REGRESSION_SIZE = 5;

	interprocess_semaphore cache_slots;
	std::atomic<double> pfs_flush;
	std::atomic<int> ssd_slots;
	std::mutex regression_mutex;
	boost::circular_buffer<double> regression;	

	monitor_t() : cache_slots(CACHE_SLOTS), pfs_flush(0), ssd_slots(0), regression(REGRESSION_SIZE) { }
    };

    managed_shared_memory segmon;
    boost::math::cubic_b_spline<double> ssd_inter;
    monitor_t *monitor = NULL;

public:
    cache_strategy_t() : segmon(open_or_create, "veloc_mon", SEGMENT_SIZE),
			 ssd_inter(ssd_y.begin(), ssd_y.end(), 6, 25) {
	monitor = segmon.find_or_construct<monitor_t>("mon_info")();
    }
    void update_pfs_flush(double chunk_throughput) {
	std::lock_guard<std::mutex> rlock(monitor->regression_mutex);
	monitor->regression.push_back(chunk_throughput);
	double sum = 0.0;
	for (auto &t: monitor->regression)
	    sum += t;
	monitor->pfs_flush.store(sum / monitor->regression.size());	
    }
    bool claim_slot() {
	if (monitor->cache_slots.try_wait())
	    return true;
	if (ssd_inter(monitor->ssd_slots.load()) < monitor->pfs_flush.load()) {
	    monitor->ssd_slots++;
	    return false;
	} else {
	    monitor->cache_slots.wait();
	    return true;
	}
    }
    void release_slot(bool cache) {
	if (cache)
	    monitor->cache_slots.post();
	else
	    monitor->ssd_slots--;
    }
};

#endif //__CACHE_STRATEGY_HPP
