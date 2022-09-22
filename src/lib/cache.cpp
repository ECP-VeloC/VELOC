#include "include/veloc/cache.hpp"
#include "common/config.hpp"
#include "common/file_util.hpp"

#include <fcntl.h>
#include <unistd.h>

#define __DEBUG
#include "common/debug.hpp"

cached_file_t::cached_file_t(const std::string &cfg_file) : async_thread(&cached_file_t::async_write, this) {
    config_t cfg(cfg_file, false);
    scratch = cfg.get("scratch");
}

void cached_file_t::async_write() {
    while (true) {
	std::unique_lock<std::mutex> lock(async_mutex);
	while (async_op_queue.size() == 0)
	    async_cond.wait(lock);
	auto cmd = async_op_queue.front();
	async_op_queue.pop_front();
	lock.unlock();
	async_cond.notify_one();
	switch (cmd.op) {
	case async_command_t::SHUTDOWN:
	    return;
	case async_command_t::WRITE:
	    file_transfer_loop(fl, cmd.offset, fr, cmd.offset, cmd.size);
	    break;
	default:
	    FATAL("internal async thread error: opcode " << cmd.op << " not recognized");
	}
    }
}

void cached_file_t::send_command(const async_command_t &cmd) {
    std::unique_lock<std::mutex> lock(async_mutex);
    while (async_op_queue.size() > MAX_QUEUE_SIZE)
	async_cond.wait(lock);
    async_op_queue.push_back(cmd);
    lock.unlock();
    async_cond.notify_one();
}

cached_file_t::~cached_file_t() {
    close();
    send_command(async_command_t(async_command_t::SHUTDOWN));
    async_thread.join();
}

bool cached_file_t::open(const std::string &fname, int flags, mode_t mode) {
    if (fl != -1 || fr != -1) {
	ERROR("cannot open file " << fname << ", reason: another file already opened");
	return false;
    }
    std::string local_fname = scratch + "/" + std::string(basename(fname.c_str()));
    fl = ::open(local_fname.c_str(), flags, mode);
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

bool cached_file_t::pread(void *buf, size_t count, off_t offset) {
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

bool cached_file_t::pwrite(const void *buf, size_t size, off_t offset) {
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
    send_command(async_command_t(async_command_t::WRITE, offset, size));
    return true;
}

void cached_file_t::flush() {
    // wait for background thread to finish async operations
    std::unique_lock<std::mutex> lock(async_mutex);
    while (async_op_queue.size() > 0)
	async_cond.wait(lock);
    lock.unlock();
    // wait for OS to finsh flushing
    fsync(fr);
}

void cached_file_t::close() {
    if (fl != -1) {
	::close(fl);
	fl = -1;
    }
    if (fr != -1) {
	flush();
	::close(fr);
	fr = -1;
    }
}
