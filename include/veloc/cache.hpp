#ifndef __VELOC_CACHED_FILE
#define __VELOC_CACHED_FILE

#include <string>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

class cached_file_t {
    static const int MAX_QUEUE_SIZE = 1024;

    struct async_command_t {
	static const int WRITE = 1, SHUTDOWN=2;
	int op;
	size_t offset, size;
	async_command_t(int _op) : op(_op) { }
	async_command_t(int _op, size_t _offset, size_t _size) :
	    op(_op), offset(_offset), size(_size) { }
    };

    std::string scratch;
    std::thread async_thread;
    std::deque<async_command_t> async_op_queue;
    std::mutex async_mutex;
    std::condition_variable async_cond;
    int fl = -1, fr = -1;

    void async_write();
    void send_command(const async_command_t &cmd);
    void flush();

public:
    cached_file_t(const std::string &cfg_file);
    ~cached_file_t();
    bool open(const std::string &fname, int flags, mode_t mode);
    bool pread(void *buf, size_t count, off_t offset);
    bool pwrite(const void *buf, size_t count, off_t offset);
    void close();
};

#endif //__VELOC_CACHED_FILE
