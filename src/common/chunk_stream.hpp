#ifndef __CHUNK_STREAM_HPP
#define __CHUNK_STREAM_HPP

#include "config.hpp"

#include <functional>

class chunk_stream_t {
public:
    typedef std::function<bool (int, bool)> callback_t;
    const size_t CHUNK_SIZE = 1 << 26; // 64 MB
private:    
    size_t c_offset = 0, chunk_no = -1;
    std::string file_name, cache_prefix;
    int fd = -1, file_mode;
    const config_t &cfg;
    callback_t callback;
    
    void next_chunk();

public:        
    chunk_stream_t(const config_t &cfg, callback_t cb);
    std::string get_chunk_name(const std::string &fname, int chunk_no, bool cached);
    void open(const std::string &fname, int mode);
    void write(char *str, size_t size);
    void read(char *str, size_t size);
    void close();
    ~chunk_stream_t();
};

#endif // __CHUNK_STREAM_HPP
