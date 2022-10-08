#ifndef __VELOC_CACHED_FILE
#define __VELOC_CACHED_FILE

#include <memory>
#include <string>

class posix_cached_file_t;

namespace veloc {
class cached_file_t {
    std::unique_ptr<posix_cached_file_t> ptr;
public:
    cached_file_t(const std::string &cfg_file);
    ~cached_file_t();
    bool open(const std::string &fname, int flags, mode_t mode);
    bool pread(void *buf, size_t count, off_t offset);
    bool pwrite(const void *buf, size_t count, off_t offset);
    void close();
    static void flush();
    static void shutdown();
};
}

#endif //__VELOC_CACHED_FILE
