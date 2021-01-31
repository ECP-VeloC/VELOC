#ifndef __THALLIUM_QUEUE_HPP
#define __THALLIUM_QUEUE_HPP

#include "status.hpp"
#include "file_util.hpp"

#include <unordered_map>
#include <chrono>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <thallium.hpp>
#include <thallium/serialization/stl/pair.hpp>

//#define __DEBUG
#include "common/debug.hpp"

namespace thallium_queue {

using namespace std::placeholders;
using namespace std::chrono_literals;
namespace tl = thallium;

typedef std::function<void (int)> completion_t;

static const std::string CHANNEL = "/dev/shm/veloc-thallium";
static const auto SLEEP_DURATION = 2s;

inline void backend_cleanup() {
    remove(CHANNEL.c_str());
}

template<typename T> struct client_queue_t {
    typedef std::list<T> list_t;
    std::mutex mutex;
    int status = VELOC_SUCCESS;
    list_t pending, progress;
};

template<typename T> class comm_client_t {
    int id;
    tl::engine engine;
    tl::remote_procedure enqueue_rpc, poll_rpc;
    tl::endpoint server;

public:
    comm_client_t(int _id) : id(_id),
                        engine("sm", THALLIUM_CLIENT_MODE),
                        enqueue_rpc(engine.define("enqueue").disable_response()),
                        poll_rpc(engine.define("poll")) {
        ssize_t csize = file_size(CHANNEL);
        if (csize == -1)
            FATAL("cannot access thallium channel: " << CHANNEL);
        char *buff = new char[csize];
        if (!read_file(CHANNEL, (unsigned char *)buff, csize))
            FATAL("cannot initialize thallium channel: " << CHANNEL);
        server = engine.lookup(buff);
        delete []buff;
    }
    int wait_completion(bool reset_status = true) {
        std::pair<int, int> reply;
        while (true) {
            reply = poll_rpc.on(server)(id, reset_status);
            if (reply.first == 0)
                break;
            std::this_thread::sleep_for(SLEEP_DURATION);
        }
        return reply.second;
    }
    void enqueue(const T &e) {
        enqueue_rpc.on(server)(e);
	DBG("enqueued element " << e);
    }
};

template<typename T> class comm_backend_t {
    typedef client_queue_t<T> container_t;
    typedef typename container_t::list_t::iterator list_iterator_t;
    std::unordered_map<int, container_t> client_map;
    std::mutex map_mutex;
    std::condition_variable map_cond;

    container_t *find_non_empty_pending() {
        // find first client with pending requests
	for (auto it = client_map.begin(); it != client_map.end(); ++it) {
	    container_t *result = &it->second;
	    if (!result->pending.empty())
		return result;
	}
	return NULL;
    }

    void set_completion(container_t *q, const list_iterator_t &it, int status) {
	// delete the element from the progress queue and notify the producer
        std::unique_lock<std::mutex> lock(map_mutex);
	DBG("completed element " << *it << ", status: " << status);
	q->progress.erase(it);
	if (q->status < 0 || status < 0)
	    q->status = std::min(q->status, status);
	else
	    q->status = std::max(q->status, status);
    }

    void start_engine() {
        tl::engine engine("na+sm://", THALLIUM_SERVER_MODE);
        std::function<void(const tl::request&, const T&)> enqueue_rpc =
            [this](const tl::request &req, const T &e) {
                std::unique_lock<std::mutex> lock(map_mutex);
                container_t &c = client_map[e.unique_id];
                c.pending.push_back(e);
                lock.unlock();
                map_cond.notify_one();
            };
        std::function<void(const tl::request&, int, bool)> poll_rpc =
            [this](const tl::request &req, int id, bool reset_status) {
                std::unique_lock<std::mutex> lock(map_mutex);
                container_t &c = client_map[id];
                std::pair<int, int> reply = std::make_pair(c.pending.size() + c.progress.size(), c.status);
                if (reset_status)
                    c.status = VELOC_SUCCESS;
                lock.unlock();
                req.respond(reply);
            };
        engine.define("enqueue", enqueue_rpc).disable_response();
        engine.define("poll", poll_rpc);
        const std::string &url = engine.self();
        DBG("thallium backend listening at: " << url);
        if (!write_file(CHANNEL, (unsigned char *)url.c_str(), url.size()))
            FATAL("cannot initialize Thallium channel: " << CHANNEL);
    }

  public:
    comm_backend_t() {
        std::thread t(&comm_backend_t<T>::start_engine, this);
        t.detach();
    }
    completion_t dequeue_any(T &e) {
	// wait until at least one pending queue has at least one element
	container_t *first_found;
        std::unique_lock<std::mutex> lock(map_mutex);
	while ((first_found = find_non_empty_pending()) == NULL)
	    map_cond.wait(lock);
	// remove the head of the pending queue and move it to the progress queue
	e = first_found->pending.front();
	first_found->pending.pop_front();
	first_found->progress.push_back(e);
	DBG("dequeued element " << e);
	return std::bind(&comm_backend_t<T>::set_completion, this, first_found, std::prev(first_found->progress.end()), _1);
    }
};

};

#endif // __THALLIUM_QUEUE_HPP
