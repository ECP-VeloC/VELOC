#include "file_provider.hpp"

#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

#if defined(WITH_URING)
#include <liburing.h>
#endif

//#define __DEBUG
#include "common/debug.hpp"

namespace file_provider {

#if defined(WITH_URING)

// One io_uring ring per thread, torn down automatically on thread exit. This
// avoids cross-thread submission races (both the engine progress thread and the
// backend workers may call in concurrently) without any locking.
namespace {
struct ring_holder_t {
    struct io_uring ring;
    bool ok = false;
    ring_holder_t() { ok = (io_uring_queue_init(64, &ring, 0) == 0); }
    ~ring_holder_t() { if (ok) io_uring_queue_exit(&ring); }
};
thread_local ring_holder_t tls_ring;

bool uring_rw(int fd, size_t offset, void *buf, size_t size, bool is_write) {
    if (!tls_ring.ok)
        return false;
    char *p = (char *)buf;
    while (size > 0) {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&tls_ring.ring);
        if (sqe == nullptr)
            return false;
        if (is_write)
            io_uring_prep_write(sqe, fd, p, size, offset);
        else
            io_uring_prep_read(sqe, fd, p, size, offset);
        if (io_uring_submit(&tls_ring.ring) < 0)
            return false;
        struct io_uring_cqe *cqe = nullptr;
        if (io_uring_wait_cqe(&tls_ring.ring, &cqe) < 0)
            return false;
        int res = cqe->res;
        io_uring_cqe_seen(&tls_ring.ring, cqe);
        if (res < 0) {
            if (res == -EINTR)
                continue;
            errno = -res;
            return false;
        }
        if (res == 0)
            return is_write; // unexpected EOF on read
        size -= res;
        offset += res;
        p += res;
    }
    return true;
}
} // anonymous namespace

bool read_range(int fd, size_t offset, void *buf, size_t size) {
    return uring_rw(fd, offset, buf, size, false);
}

bool write_range(int fd, size_t offset, const void *buf, size_t size) {
    return uring_rw(fd, offset, const_cast<void *>(buf), size, true);
}

bool copy_range(int src_fd, size_t src_off, int dst_fd, size_t dst_off, size_t size) {
    const size_t MAX_BUFF_SIZE = 1 << 24;
    char *buff = new char[MAX_BUFF_SIZE];
    bool success = true;
    while (size > 0) {
        size_t chunk = std::min(MAX_BUFF_SIZE, size);
        if (!read_range(src_fd, src_off, buff, chunk) || !write_range(dst_fd, dst_off, buff, chunk)) {
            success = false;
            break;
        }
        size -= chunk;
        src_off += chunk;
        dst_off += chunk;
    }
    delete[] buff;
    return success;
}

#elif defined(WITH_POSIX_DIRECT) || defined(WITH_POSIX_RW)

bool read_range(int fd, size_t offset, void *buf, size_t size) {
    char *p = (char *)buf;
    while (size > 0) {
        ssize_t r = pread(fd, p, size, offset);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (r == 0)
            return false; // unexpected EOF
        size -= r;
        offset += r;
        p += r;
    }
    return true;
}

bool write_range(int fd, size_t offset, const void *buf, size_t size) {
    const char *p = (const char *)buf;
    while (size > 0) {
        ssize_t w = pwrite(fd, p, size, offset);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        size -= w;
        offset += w;
        p += w;
    }
    return true;
}

#if defined(WITH_POSIX_DIRECT)
bool copy_range(int src_fd, size_t src_off, int dst_fd, size_t dst_off, size_t size) {
    while (size > 0) {
        ssize_t transferred = copy_file_range(src_fd, (off64_t *)&src_off, dst_fd, (off64_t *)&dst_off, size, 0);
        if (transferred == -1) {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (transferred == 0)
            break;
        size -= transferred;
    }
    return size == 0;
}
#else // WITH_POSIX_RW
bool copy_range(int src_fd, size_t src_off, int dst_fd, size_t dst_off, size_t size) {
    const size_t MAX_BUFF_SIZE = 1 << 24;
    char *buff = new char[MAX_BUFF_SIZE];
    bool success = true;
    while (size > 0) {
        size_t chunk = std::min(MAX_BUFF_SIZE, size);
        if (!read_range(src_fd, src_off, buff, chunk) || !write_range(dst_fd, dst_off, buff, chunk)) {
            success = false;
            break;
        }
        size -= chunk;
        src_off += chunk;
        dst_off += chunk;
    }
    delete[] buff;
    return success;
}
#endif

#else
#error Invalid FILE_IO transfer method selected. Valid choices: WITH_POSIX_DIRECT, WITH_POSIX_RW, WITH_URING
#endif

} // namespace file_provider
