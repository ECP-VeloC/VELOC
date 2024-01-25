#include "include/veloc/cache.hpp"
#include "posix_cache.hpp"
#include "common/file_util.hpp"

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>

//#define __DEBUG
#include "common/debug.hpp"

// async background I/O implementation
struct async_command_t {
    static const int WRITE = 1, CLOSE = 2;
    int op, fl, fr;
    size_t offset, size;
    async_command_t(int _op) : op(_op) { }
    async_command_t(int _op, int _fl, int _fr) :
        op(_op), fl(_fl), fr(_fr) { }
    async_command_t(int _op, int _fl, int _fr, size_t _offset, size_t _size) :
        op(_op), fl(_fl), fr(_fr), offset(_offset), size(_size) { }
};

struct async_context_t {
    static const size_t DEFAULT_QUEUE_SIZE = 1 << 16;
    size_t MAX_QUEUE_SIZE;
    std::mutex async_mutex;
    std::condition_variable async_cond;
    std::deque<async_command_t> async_op_queue;
    std::thread async_thread;
    bool started = false, finished = false, fail_status = false;
};

static async_context_t static_context;

static void async_write() {
    cpu_set_t cpu_mask;
    // add all online CPUs to the CPU mask
    CPU_ZERO(&cpu_mask);
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    for (int i = 0; i < nproc; i++)
        CPU_SET(i, &cpu_mask);
    // schedule background thread on CPU mask and reduce its priority
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
    nice(10);

    while (true) {
	std::unique_lock<std::mutex> lock(static_context.async_mutex);
	while (static_context.async_op_queue.size() == 0 && !static_context.finished)
	    static_context.async_cond.wait(lock);
        if (static_context.async_op_queue.size() == 0 && static_context.finished)
            break;
	auto cmd = static_context.async_op_queue.front();
	static_context.async_op_queue.pop_front();
	lock.unlock();
	static_context.async_cond.notify_all();
	switch (cmd.op) {
        case async_command_t::CLOSE:
            DBG("close, fl = " << cmd.fl << ", fr = " << cmd.fr);
	    close(cmd.fl);
	    close(cmd.fr);
	    break;
	case async_command_t::WRITE:
	    DBG("async write, fl = " << cmd.fl << ", fr = " << cmd.fr << ", offset = " << cmd.offset << ", size = " << cmd.size);
	    if (!file_transfer_loop(cmd.fl, cmd.offset, cmd.fr, cmd.offset, cmd.size)) {
		ERROR("async write failed, error = " << std::strerror(errno));
		std::unique_lock<std::mutex> lock(static_context.async_mutex);
		static_context.fail_status = true;
	    }
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
    if (!static_context.finished)
        static_context.async_op_queue.push_back(cmd);
    lock.unlock();
    static_context.async_cond.notify_all();
}

// actual client implementation
posix_cached_file_t::posix_cached_file_t(const std::string &scratch_path) : scratch(scratch_path) {
    if (!check_dir(scratch))
        FATAL("scratch directory " << scratch << " inaccessible!");
    std::unique_lock<std::mutex> lock(static_context.async_mutex);
    char *env = getenv("VELOC_POSIX_CACHE_SIZE");
    if (env != NULL) {
	std::stringstream ss(env);
	ss >> static_context.MAX_QUEUE_SIZE;
	if (ss.fail())
	    static_context.MAX_QUEUE_SIZE = static_context.DEFAULT_QUEUE_SIZE;
    } else
	static_context.MAX_QUEUE_SIZE = static_context.DEFAULT_QUEUE_SIZE;
    if (!static_context.started) {
	std::thread(async_write).detach();
	static_context.started = true;
    }
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

// PIMPL wrappers
namespace veloc {
cached_file_t::cached_file_t(const std::string &scratch_path) : ptr(new ::posix_cached_file_t(scratch_path)) {}
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
bool cached_file_t::flush() {
    // wait for background thread to finish async operations
    std::unique_lock<std::mutex> lock(static_context.async_mutex);
    while (static_context.async_op_queue.size() > 0)
	static_context.async_cond.wait(lock);
    bool ret = static_context.fail_status;
    static_context.fail_status = false;
    return ret;
}
void cached_file_t::shutdown() {
    std::unique_lock<std::mutex> lock(static_context.async_mutex);
    static_context.finished = true;
    static_context.async_cond.notify_all();
}
}
