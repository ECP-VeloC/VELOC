#ifndef __POSIX_VELOC_CACHED_FILE
#define __POSIX_VELOC_CACHED_FILE

#include <string>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

class posix_cached_file_t {
    static const int MAX_QUEUE_SIZE = 1024;

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

    std::string scratch;
    std::deque<async_command_t> async_op_queue;
    std::mutex async_mutex;
    std::condition_variable async_cond;
    std::thread async_thread;
    int fl = -1, fr = -1;

    void async_write();
    void send_command(const async_command_t &cmd);
    void flush();

public:
    posix_cached_file_t(const std::string &cfg_file);
    ~posix_cached_file_t();
    bool open(const std::string &fname, int flags, mode_t mode);
    bool pread(void *buf, size_t count, off_t offset);
    bool pwrite(const void *buf, size_t count, off_t offset);
    void close();
};

#endif //__POSIX_VELOC_CACHED_FILE
