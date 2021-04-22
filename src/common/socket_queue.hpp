#ifndef __SOCKET_QUEUE_HPP
#define __SOCKET_QUEUE_HPP

#include "status.hpp"
#include "command.hpp"

#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <list>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

namespace socket_queue {

using namespace std::placeholders;

typedef std::function<void (int)> completion_t;

static const std::string CHANNEL = "/dev/shm/veloc-socket-" + std::to_string(getuid());
static const int MAX_CLIENTS = 256;

inline void backend_cleanup() {
    remove(CHANNEL.c_str());
}

struct client_queue_t {
    typedef std::list<command_t> list_t;
    int status = VELOC_SUCCESS;
    list_t pending, progress;
    int fd = -1;
    bool waiting = false, reset_status = true;
};

static inline void fatal_comm() {
    FATAL("cannot interact with unix socket: " << CHANNEL << "; error = " << std::strerror(errno));
}

template<typename T> class comm_client_t {
    int id, fd;

public:
    comm_client_t(int _id) : id(_id) {
        sockaddr_un addr;
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
            fatal_comm();
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strcpy(addr.sun_path, CHANNEL.c_str());
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
            fatal_comm();
    }

    int wait_completion(bool reset_status = true) {
        command_t c(id, command_t::STATUS, reset_status, "");
        int reply;
        if (write(fd, &c, sizeof(c)) != sizeof(c))
            fatal_comm();
        if (read(fd, &reply, sizeof(reply)) != sizeof(reply))
            fatal_comm();
        return reply;
    }

    void enqueue(const command_t &c) {
        if (write(fd, &c, sizeof(c)) != sizeof(c))
            fatal_comm();
	DBG("enqueued element " << c);
    }
};

template<typename T> class comm_backend_t {
    typedef client_queue_t container_t;
    typedef typename container_t::list_t::iterator list_iterator_t;
    std::unordered_map<int, container_t> client_map;
    std::mutex map_mutex;
    std::condition_variable map_cond;
    int fd;

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
        DBG("completed element " << *it << ", status: " << status << ", waiting = " << q->waiting);
	q->progress.erase(it);
	if (q->status < 0 || status < 0)
	    q->status = std::min(q->status, status);
	else
	    q->status = std::max(q->status, status);
        send_wait_reply(q);
    }

    // safety check: must be called with unique_lock acquired
    void send_wait_reply(container_t *q) {
        if (q->waiting && q->pending.empty() && q->progress.empty()) {
            int reply = q->status;
            if(q->reset_status)
                q->status = VELOC_SUCCESS;
            q->waiting = false;
            if (write(q->fd, &reply, sizeof(reply)) != sizeof(reply))
                fatal_comm();
        }
    }

    void enqueue(const command_t &e, int sock) {
        std::unique_lock<std::mutex> lock(map_mutex);
        container_t *q = &client_map[e.unique_id];
        q->fd = sock;
        if (e.command == command_t::STATUS) {
            q->waiting = true;
            q->reset_status = e.version;
            send_wait_reply(q);
        } else {
            q->pending.push_back(e);
            lock.unlock();
            map_cond.notify_one();
        }
    }

    void handle_connections() {
        pollfd fds[MAX_CLIENTS];
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        int fds_size = 1;

        while (true) {
            int nready = poll(fds, fds_size, -1);
            DBG("fds_size = " << nready);
            if (nready == -1)
                fatal_comm();
            // loop over all existing client connections
            for (int i = 1; i < fds_size; i++)
                if (fds[i].revents & POLLIN) {
                    command_t c;
                    if (read(fds[i].fd, &c, sizeof(c)) == sizeof(c))
                        enqueue(c, fds[i].fd);
                    else // client has closed connection, maybe died
                        fds[i].fd = -1;
                }
            // check if we have a new client connection
            if (fds[0].revents & POLLIN) {
                int client = accept(fd, NULL, NULL);
                if (client == -1)
                    fatal_comm();
                int i = 0;
                while (i < fds_size && fds[i].fd != -1)
                    i++;
                if (i > MAX_CLIENTS)
                    FATAL("maximum number of clients (" << MAX_CLIENTS << ") exceeded for unix socket: " << CHANNEL);
                fds[i].fd = client;
                fds[i].events = POLLIN;
                fds_size = std::max(fds_size, i + 1);
            }
        }
    }

  public:
    comm_backend_t() {
        sockaddr_un addr;
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
            fatal_comm();
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        std::strcpy(addr.sun_path, CHANNEL.c_str());
        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
            fatal_comm();
        if (listen(fd, MAX_CLIENTS) == -1)
            fatal_comm();
        std::thread t(&comm_backend_t<T>::handle_connections, this);
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

#endif // __SOCKET_QUEUE_HPP
