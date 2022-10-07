#ifndef __POSIX_VELOC_CACHED_FILE
#define __POSIX_VELOC_CACHED_FILE

#include <string>

class posix_cached_file_t {
    std::string scratch;
    int fl = -1, fr = -1;

public:
    posix_cached_file_t(const std::string &cfg_file);
    ~posix_cached_file_t();
    bool open(const std::string &fname, int flags, mode_t mode);
    bool pread(void *buf, size_t count, off_t offset);
    bool pwrite(const void *buf, size_t count, off_t offset);
    void close();
};

#endif //__POSIX_VELOC_CACHED_FILE
