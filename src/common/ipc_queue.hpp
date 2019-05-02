#ifndef __IPC_QUEUE_HPP
#define __IPC_QUEUE_HPP

#include "status.hpp"

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/containers/list.hpp>

#include <functional>
using namespace std::placeholders;

//#define __DEBUG
#include "common/debug.hpp"

namespace veloc_ipc {

using namespace boost::interprocess;

typedef std::function<void (int)> completion_t;

inline void cleanup() {
    boost::interprocess::shared_memory_object::remove("veloc_mon");
    boost::interprocess::shared_memory_object::remove("veloc_shm");
    boost::interprocess::named_mutex::remove("veloc_pending_mutex");
    boost::interprocess::named_condition::remove("veloc_pending_cond");
}
    
template <class T> class shm_queue_t {
    static const size_t MAX_SIZE = 1 << 20;
    struct container_t {
	typedef allocator<T, managed_shared_memory::segment_manager> T_allocator;
	typedef list<T, T_allocator> list_t;
	
	interprocess_mutex mutex;
	interprocess_condition cond;
	int status = VELOC_SUCCESS;
	list_t pending, progress;
	container_t(const T_allocator &alloc) : pending(alloc), progress(alloc) { }
    };
    typedef typename container_t::list_t::iterator list_iterator_t;

    managed_shared_memory segment;
    named_mutex     pending_mutex;
    named_condition pending_cond;
    container_t *data = NULL;

    container_t *find_non_empty_pending() {
	for (managed_shared_memory::const_named_iterator it = segment.named_begin(); it != segment.named_end(); ++it) {
	    container_t *result = (container_t *)it->value();
	    if (!result->pending.empty())
		return result;
	}
	return NULL;
    }
    bool check_completion() {
	// this is a predicate intended to be used for condition variables only
	return data->pending.empty() && data->progress.empty();
    }
    void set_completion(container_t *q, const list_iterator_t  &it, int status) {
	// delete the element from the progress queue and notify the producer
	scoped_lock<interprocess_mutex> queue_lock(q->mutex);
	DBG("completed element " << *it);
	q->progress.erase(it);
	if (q->status < 0 || status < 0)
	    q->status = std::min(q->status, status);
	else
	    q->status = std::max(q->status, status);
	q->cond.notify_one();
    }
    
  public:    
    shm_queue_t(const char *id) : segment(open_or_create, "veloc_shm" , MAX_SIZE),
				  pending_mutex(open_or_create, "veloc_pending_mutex"),
				  pending_cond(open_or_create, "veloc_pending_cond") {
	scoped_lock<named_mutex> cond_lock(pending_mutex);
	if (id != NULL)
	    data = segment.find_or_construct<container_t>(id)(segment.get_allocator<typename container_t::T_allocator>());
    }
    int wait_completion(bool reset_status = true) {
	scoped_lock<interprocess_mutex> cond_lock(data->mutex);
	while (!check_completion())
	    data->cond.wait(cond_lock);
	int ret = data->status;
	if (reset_status)
	    data->status = VELOC_SUCCESS;
	return ret;
    }
    void enqueue(const T &e) {
	// enqueue an element and notify the consumer
	scoped_lock<interprocess_mutex> queue_lock(data->mutex);
	data->pending.push_back(e);
	queue_lock.unlock();
	scoped_lock<named_mutex> cond_lock(pending_mutex);
	pending_cond.notify_one();
	DBG("enqueued element " << e);
    }
    completion_t dequeue_any(T &e) {
	// wait until at least one pending queue has at least one element
	container_t *first_found;
	scoped_lock<named_mutex> cond_lock(pending_mutex);
	while ((first_found = find_non_empty_pending()) == NULL)
	    pending_cond.wait(cond_lock);
	// remove the head of the pending queue and move it to the progress queue
	scoped_lock<interprocess_mutex> queue_lock(first_found->mutex);
	e = first_found->pending.front();
	first_found->pending.pop_front();
	first_found->progress.push_back(e);
	DBG("dequeued element " << e);
	return std::bind(&shm_queue_t<T>::set_completion, this, first_found, std::prev(first_found->progress.end()), _1);
    }
    size_t get_num_queues() {
	return segment.get_num_named_objects();
    }
};

};

#endif // __IPC_QUEUE_HPP
