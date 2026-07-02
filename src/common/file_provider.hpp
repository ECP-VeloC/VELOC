#ifndef __FILE_PROVIDER_HPP
#define __FILE_PROVIDER_HPP

#include <cstddef>

// Low-level file transfer mechanism shared across the whole project (client and
// backend). The concrete implementation is selected at compile time via FILE_IO:
//   WITH_POSIX_DIRECT -> copy_file_range for file->file, pread/pwrite otherwise
//   WITH_POSIX_RW     -> pread/pwrite loops
//   WITH_URING        -> liburing (io_uring) submissions
//
// All calls are blocking with respect to the caller; the transfer engine runs
// them on its progress thread so the application thread is never blocked.
namespace file_provider {

// file -> host buffer
bool read_range(int fd, size_t offset, void *buf, size_t size);
// host buffer -> file
bool write_range(int fd, size_t offset, const void *buf, size_t size);
// file -> file (used by the backend flush / restore path)
bool copy_range(int src_fd, size_t src_off, int dst_fd, size_t dst_off, size_t size);

} // namespace file_provider

#endif // __FILE_PROVIDER_HPP
