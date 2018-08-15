#include "chunk_stream.hpp"

#define __DEBUG
#include "common/debug.hpp"

chunk_stream_t::chunk_stream_t(const config_t &c, callback_t cb) : cfg(c), callback(cb) {
    if (!cfg.get_optional("cache", cache_prefix))
	cache_prefix = cfg.get("scratch");
    c_stream.exceptions(std::ios::failbit | std::ios::badbit);
}

void chunk_stream_t::open(const std::string &fname, std::ios::openmode fmode) {
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
    if (c_stream.is_open())
	c_stream.close();
    bool cached = callback(chunk_no, false);
    chunk_no++;    
    c_stream.open(get_chunk_name(file_name, chunk_no, cached), file_mode);
    c_offset = 0;
}

void chunk_stream_t::write(char *str, size_t size) {
    size_t next, written = 0;
    do {
	next = std::min(size - written, CHUNK_SIZE - c_offset);
	if (next == 0) {
	    next_chunk();
	    continue;
	}
        c_stream.write(str + written, next);
	written += next;
	c_offset += next;
    } while (written < size);
}

void chunk_stream_t::read(char *str, size_t size) {
    size_t next, read = 0;
    do {
	next = std::min(size - read, CHUNK_SIZE - c_offset);
	if (next == 0) {
	    next_chunk();
	    continue;
	}
	c_stream.read(str + read, next);
	read += next;
	c_offset += next;
    } while (read < size);
}

void chunk_stream_t::close() {
    if (c_stream.is_open()) {
	if (c_offset > 0)
	    callback(chunk_no, true);
	c_stream.close();
    }
}

chunk_stream_t::~chunk_stream_t() {
    close();
}
