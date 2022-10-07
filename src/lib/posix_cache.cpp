#include "include/veloc/cache.hpp"
#include "posix_cache.hpp"
#include "common/config.hpp"
#include "common/file_util.hpp"

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fcntl.h>
#include <unistd.h>

#define __DEBUG
#include "common/debug.hpp"

// async background I/O implementation
struct async_command_t {
    static const int SHUTDOWN = 1, WRITE = 2, CLOSE = 3;
    int op, fl, fr;
    size_t offset, size;
    async_command_t(int _op) : op(_op) { }
    async_command_t(int _op, int _fl, int _fr) :
        op(_op), fl(_fl), fr(_fr) { }
    async_command_t(int _op, int _fl, int _fr, size_t _offset, size_t _size) :
        op(_op), fl(_fl), fr(_fr), offset(_offset), size(_size) { }
};

struct async_context_t {
    static const int MAX_QUEUE_SIZE = 128;
    std::mutex async_mutex;
    std::condition_variable async_cond;
    std::deque<async_command_t> async_op_queue;
    std::thread async_thread;
};

static async_context_t static_context;

static void async_write() {
    while (true) {
	std::unique_lock<std::mutex> lock(static_context.async_mutex);
	while (static_context.async_op_queue.size() == 0)
	    static_context.async_cond.wait(lock);
	auto cmd = static_context.async_op_queue.front();
	static_context.async_op_queue.pop_front();
	lock.unlock();
	static_context.async_cond.notify_one();
	switch (cmd.op) {
	case async_command_t::SHUTDOWN:
	    return;
        case async_command_t::CLOSE:
            DBG("close, fl = " << cmd.fl << ", fr = " << cmd.fr);
	    close(cmd.fl);
	    close(cmd.fr);
	    break;
	case async_command_t::WRITE:
	    DBG("async write, fl = " << cmd.fl << ", fr = " << cmd.fr << ", offset = " << cmd.offset << ", size = " << cmd.size);
	    file_transfer_loop(cmd.fl, cmd.offset, cmd.fr, cmd.offset, cmd.size);
	    break;
	default:
	    FATAL("internal async thread error: opcode " << cmd.op << " not recognized");
	}
    }
}

static void send_command(const async_command_t &cmd) {
    std::unique_lock<std::mutex> lock(static_context.async_mutex);
    while (static_context.async_op_queue.size() > static_context.MAX_QUEUE_SIZE)
	static_context.async_cond.wait(lock);
    static_context.async_op_queue.push_back(cmd);
    lock.unlock();
    static_context.async_cond.notify_one();
}

// PIMPL wrappers
namespace veloc {
cached_file_t::cached_file_t(const std::string &cfg_file) : ptr(new ::posix_cached_file_t(cfg_file)) {}
cached_file_t::~cached_file_t() = default;
bool cached_file_t::open(const std::string &fname, int flags, mode_t mode) {
    return ptr->open(fname, flags, mode);
}
bool cached_file_t::pread(void *buf, size_t count, off_t offset) {
    return ptr->pread(buf, count, offset);
}
bool cached_file_t::pwrite(const void *buf, size_t count, off_t offset) {
    return ptr->pwrite(buf, count, offset);
}
void cached_file_t::close() {
    ptr->close();
}
void cached_file_t::flush() {
    TIMER_START(flush_timer);
    // wait for background thread to finish async operations
    std::unique_lock<std::mutex> lock(static_context.async_mutex);
    while (static_context.async_op_queue.size() > 0)
	static_context.async_cond.wait(lock);
    lock.unlock();
    TIMER_STOP(flush_timer, "wait for async I/O background thread finished");
}
}

// actual client implementation
posix_cached_file_t::posix_cached_file_t(const std::string &cfg_file) {
    config_t cfg(cfg_file, false);
    scratch = cfg.get("scratch");
    std::unique_lock<std::mutex> lock(static_context.async_mutex);
    if (static_context.async_thread.joinable())
        return;
    static_context.async_thread = std::thread(async_write);
}

posix_cached_file_t::~posix_cached_file_t() {
    close();
}

bool posix_cached_file_t::open(const std::string &fname, int flags, mode_t mode) {
    if (fl != -1 || fr != -1) {
	ERROR("cannot open file " << fname << ", reason: another file already opened");
	return false;
    }
    std::string local_fname = scratch + "/" + std::string(basename(fname.c_str()));
    fl = ::open(local_fname.c_str(), O_RDWR | O_CREAT, mode);
    if (fl == -1) {
	ERROR("cannot open local file " << fname << ", reason: " << strerror(errno));
	return false;
    }
    fr = ::open(fname.c_str(), flags, mode);
    if (fr == -1) {
	ERROR("cannot open remote file " << fname << ", reason: " << strerror(errno));
	return false;
    }
    return true;
}

bool posix_cached_file_t::pread(void *buf, size_t count, off_t offset) {
    if (fl == -1 || fr == -1) {
	ERROR("no file open yet, operation cannot be performed");
	return false;
    }
    while (count > 0) {
	ssize_t br = ::pread(fr, buf, count, offset);
	if (br <= 0) {
	    ERROR("reading " << count << " bytes from offset " << offset << " failed, reason: " << strerror(errno));
	    return false;
	}
	count -= br;
	offset += br;
    }
    return true;
}

bool posix_cached_file_t::pwrite(const void *buf, size_t size, off_t offset) {
    if (fl == -1 || fr == -1) {
	ERROR("no file open yet, operation cannot be performed");
	return false;
    }
    size_t count = size, off = offset;
    while (count > 0) {
	ssize_t br = ::pwrite(fl, buf, count, off);
	if (br == -1) {
	    ERROR("writing " << count << " bytes from offset " << off << " failed, reason: " << strerror(errno));
	    return false;
	}
	count -= br;
	off += br;
    }
    send_command(async_command_t(async_command_t::WRITE, fl, fr, offset, size));
    return true;
}

void posix_cached_file_t::close() {
    if (fl != -1 && fr != -1) {
        send_command(async_command_t(async_command_t::CLOSE, fl, fr));
        fl = -1;
        fr = -1;
    }
}
