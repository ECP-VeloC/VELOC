#ifndef __IPC_QUEUE_HPP
#define __IPC_QUEUE_HPP

#include <cstring>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/containers/deque.hpp>

#define __DEBUG
#include "common/debug.hpp"

class command_t {
public:
    static const size_t MAX_SIZE = 128;
    static const int CHECKPOINT_END = 0, RESTART_BEGIN = 1;
    
    int command;
    int version;
    char ckpt_name[MAX_SIZE];

    command_t() { }
    command_t(int c, int v, const std::string &s) : command(c), version(v) {
	std::strcpy(ckpt_name, s.c_str());
    }
    friend std::ostream &operator<<(std::ostream &output, const command_t &c) {
	output << "(Command = '" << c.command << "', Version = '" << c.version << "', File = '" << c.ckpt_name << "')";
	return output;
    }
};

namespace veloc_ipc {
using namespace boost::interprocess;
static const char shm_name[] = "veloc_shm";
template <class T> class shm_queue_t {
    static const size_t MAX_SIZE = 1 << 20;
    
    struct container_t {
	typedef allocator<T, managed_shared_memory::segment_manager> T_allocator;
	typedef deque<T, T_allocator> queue_t;
	
	interprocess_mutex mutex;
	interprocess_condition wait_cond;
	queue_t queue;
	container_t(const T_allocator &alloc) : queue(alloc) { }
    };

    managed_shared_memory segment;
    named_mutex     cond_mutex;
    named_condition cond;
    container_t *data = NULL;

    bool find_non_empty_queue(std::string &id) {
	for (managed_shared_memory::const_named_iterator it = segment.named_begin(); it != segment.named_end(); ++it) {	    
	    data = (container_t *)it->value();
	    if (!data->queue.empty()) {
		id = std::string(it->name(), it->name_length());
		return true;
	    }
	}
	return false;
    }
    
  public:
    shm_queue_t() : segment(open_or_create, shm_name, MAX_SIZE),
		    cond_mutex(open_or_create, "backend_cond_mutex"),
		    cond(open_or_create, "backend_cond") { }        
    void set_id(const std::string &id) {
	data = segment.find_or_construct<container_t>(id.c_str())(segment.get_allocator<typename container_t::T_allocator>());
    }
    void enqueue(T &e) {
	assert(data != NULL);
	scoped_lock<interprocess_mutex> lock(data->mutex);
	data->queue.push_back(e);
	lock.unlock();
	cond.notify_one();
    }
    void dequeue(T &e) {
	assert(data != NULL);
	// wait until at the queue for the registered id has at least one element
	scoped_lock<named_mutex> cond_lock(cond_mutex);
	while (data->queue.empty())
	    cond.wait(cond_lock);
	cond_lock.unlock();
	// remove the head of the queue
	scoped_lock<interprocess_mutex> queue_lock(data->mutex);
	e = data->queue.front();
	data->queue.pop_front();
    }
    std::string dequeue_any(T &e) {
	std::string id;
	// wait until at least one queue has at least one element
	scoped_lock<named_mutex> cond_lock(cond_mutex);
	while (!find_non_empty_queue(id))
	    cond.wait(cond_lock);
	cond_lock.unlock();
	// remove the head of the queue
	scoped_lock<interprocess_mutex> queue_lock(data->mutex);
	e = data->queue.front();
	data->queue.pop_front();
	return id;
    }
};
};

#endif // __IPC_QUEUE_HPP
