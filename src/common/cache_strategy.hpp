#ifndef __CACHE_STRATEGY_HPP
#define __CACHE_STRATEGY_HPP

#include <atomic>
#include <mutex>
#include <cstdlib>
#include <cstdio>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/math/interpolators/cubic_b_spline.hpp>
#include <boost/circular_buffer.hpp>

#define __DEBUG
#include "common/debug.hpp"

using namespace boost::interprocess;

class cache_strategy_t {
    const unsigned int SEGMENT_SIZE = 1 << 16, REGRESSION_SIZE = 5;
    const std::vector<double> ssd_y { 0.43, 1.55, 2.07, 3.93, 4.92, 5.61, 6.54, 7.14, 8.28, 9.03, 10.02, 11.08, 11.86, 12.61, 13.78, 14.83, 16.25, 18.52, 19.02, 20.90, 21.88, 24.69, 27.51, 30.01, 32.8, 35.2 };

    struct monitor_t {
	interprocess_semaphore cache_slots;
	std::atomic<double> pfs_flush;
	std::atomic<int> ssd_slots;
	monitor_t(unsigned int slots) : cache_slots(slots), pfs_flush(0), ssd_slots(0) { }
    };

    managed_shared_memory segmon;
    boost::math::cubic_b_spline<double> ssd_inter;
    monitor_t *monitor = NULL;
    std::mutex regression_mutex;
    boost::circular_buffer<double> regression;
    int naive_active = 0;

public:
    cache_strategy_t() : segmon(open_or_create, "veloc_mon", SEGMENT_SIZE),
			 ssd_inter(ssd_y.begin(), ssd_y.end(), 1, 10), regression(REGRESSION_SIZE) {
	char *str_size = std::getenv("VELOC_CACHE_SLOTS");
	unsigned int size = 32;
	if (str_size != NULL)
	    sscanf(str_size, "%u", &size);
	str_size = std::getenv("NAIVE_ACTIVE");
	if (str_size != NULL)
	    sscanf(str_size, "%d", &naive_active);
	DBG("cache settings: slots = " << size << ", naive_active = " << naive_active);
	monitor = segmon.find_or_construct<monitor_t>("mon_info")(size);
    }
    void update_pfs_flush(double chunk_throughput) {
	regression_mutex.lock();
	regression.push_back(chunk_throughput);
	double sum = 0.0;
	for (auto &t: regression)
	    sum += t;
	monitor->pfs_flush.store(sum / regression.size());
	regression_mutex.unlock();
    }
    bool claim_slot() {
	if (monitor->cache_slots.try_wait())
	    return true;
	if (naive_active || //(1 << 28) / (monitor->ssd_slots + 1) > monitor->pfs_flush.load()) {
	(1 << 26) / ssd_inter(monitor->ssd_slots.load()) > monitor->pfs_flush.load()) {
	    monitor->ssd_slots++;
	    return false;
	} else {
	    monitor->cache_slots.wait();
	    return true;
	}
    }
    void release_cache() {
	monitor->cache_slots.post();
    }
    void release_ssd() {
	monitor->ssd_slots--;
    }
};

#endif //__CACHE_STRATEGY_HPP
