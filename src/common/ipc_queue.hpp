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

namespace ipc_queue {

using namespace boost::interprocess;

typedef std::function<void (int)> completion_t;

static const size_t IPC_MAX_SIZE = 1 << 20;

inline void backend_cleanup() {
    shared_memory_object::remove("veloc_shm");
    named_mutex::remove("veloc_pending_mutex");
    named_condition::remove("veloc_pending_cond");
}

template<typename T> struct client_queue_t {
    typedef allocator<T, managed_shared_memory::segment_manager> T_allocator;
    typedef list<T, T_allocator> list_t;

    interprocess_mutex mutex;
    interprocess_condition cond;
    int status = VELOC_SUCCESS;
    list_t pending, progress;
    client_queue_t(const T_allocator &alloc) : pending(alloc), progress(alloc) { }
};

template<typename T> class comm_client_t {
    typedef client_queue_t<T> container_t;
    managed_shared_memory segment;
    named_mutex     pending_mutex;
    named_condition pending_cond;
    container_t *data = NULL;

  public:
    comm_client_t(int id) : segment(open_or_create, "veloc_shm" , IPC_MAX_SIZE),
                       pending_mutex(open_or_create, "veloc_pending_mutex"),
                       pending_cond(open_or_create, "veloc_pending_cond") {
	scoped_lock<named_mutex> cond_lock(pending_mutex);
        data = segment.find_or_construct<container_t>(std::to_string(id).c_str())(segment.get_allocator<typename container_t::T_allocator>());
    }
    int wait_completion(bool reset_status = true) {
	scoped_lock<interprocess_mutex> cond_lock(data->mutex);
	while (!data->pending.empty() || !data->progress.empty())
	    data->cond.wait(cond_lock);
	int ret = data->status;
        DBG("wait completion returning: " << ret);
	if (reset_status)
	    data->status = VELOC_SUCCESS;
	return ret;
    }
    void enqueue(const T &e) {
	// enqueue an element and notify the consumer
	scoped_lock<interprocess_mutex> queue_lock(data->mutex);
	data->pending.push_back(e);
	queue_lock.unlock();
	pending_cond.notify_one();
	DBG("enqueued element " << e);
    }
};

template<typename T> class comm_backend_t {
    typedef client_queue_t<T> container_t;
    typedef typename container_t::list_t::iterator list_iterator_t;
    managed_shared_memory segment;
    named_mutex     pending_mutex;
    named_condition pending_cond;

    container_t *find_non_empty_pending() {
	for (managed_shared_memory::const_named_iterator it = segment.named_begin(); it != segment.named_end(); ++it) {
	    container_t *result = (container_t *)it->value();
	    if (!result->pending.empty())
		return result;
	}
	return NULL;
    }
    void set_completion(container_t *q, const list_iterator_t  &it, int status) {
	// delete the element from the progress queue and notify the producer
	scoped_lock<interprocess_mutex> queue_lock(q->mutex);
	DBG("completed element " << *it << ", status: " << status);
	q->progress.erase(it);
	if (q->status < 0 || status < 0)
	    q->status = std::min(q->status, status);
	else
	    q->status = std::max(q->status, status);
        queue_lock.unlock();
	q->cond.notify_one();
    }

  public:
    comm_backend_t() : segment(open_or_create, "veloc_shm" , IPC_MAX_SIZE),
                 pending_mutex(open_or_create, "veloc_pending_mutex"),
                 pending_cond(open_or_create, "veloc_pending_cond") { }
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
	return std::bind(&comm_backend_t<T>::set_completion, this, first_found, std::prev(first_found->progress.end()), _1);
    }
};

};

#endif // __IPC_QUEUE_HPP
