#include "chunk_stream.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

#define __DEBUG
#include "common/debug.hpp"

chunk_stream_t::chunk_stream_t(const config_t &c, callback_t cb) : cfg(c), callback(cb) {
    if (!cfg.get_optional("cache", cache_prefix))
	cache_prefix = cfg.get("scratch");
}

void chunk_stream_t::open(const std::string &fname, int fmode) {
    file_name = fname;
    file_mode = fmode;
    next_chunk();
}

std::string chunk_stream_t::get_chunk_name(const std::string &fname, int chunk_id, bool cached) {
    if (cached)
	return cache_prefix + "/" + fname + "." + std::to_string(chunk_id);
    else
	return cfg.get("scratch") + "/" + fname + "." + std::to_string(chunk_id);
}

void chunk_stream_t::next_chunk() {
    ::close(fd);
    bool cached = callback(chunk_no, false);
    chunk_no++;
    fd = ::open(get_chunk_name(file_name, chunk_no, cached).c_str(), file_mode | O_SYNC, 0644);
    if (fd == -1)
	throw std::runtime_error("failed to open: " + file_name);
    c_offset = 0;
}

void chunk_stream_t::write(char *str, size_t size) {
    ssize_t next;
    size_t written = 0;
    do {
	next = std::min(size - written, CHUNK_SIZE - c_offset);
	if (next == 0) {
	    next_chunk();
	    continue;
	}
	next = ::write(fd, str + written, next);
	if (next == -1)
	    throw std::runtime_error("failed to write to " + file_name);
	written += next;
	c_offset += next;
    } while (written < size);
}

void chunk_stream_t::read(char *str, size_t size) {
    ssize_t next;
    size_t read = 0;
    do {
	next = std::min(size - read, CHUNK_SIZE - c_offset);
	if (next == 0) {
	    next_chunk();
	    continue;
	}
	next = ::read(fd, str + read, next);
	if (next == -1)
	    throw std::runtime_error("failed to read from " + file_name);
	read += next;
	c_offset += next;
    } while (read < size);
}

void chunk_stream_t::close() {
    if (c_offset > 0)
	callback(chunk_no, true);
    ::close(fd);
}

chunk_stream_t::~chunk_stream_t() {
    close();
}
