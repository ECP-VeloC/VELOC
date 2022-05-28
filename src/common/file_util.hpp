#ifndef __FILE_UTIL
#define __FILE_UTIL

#include "status.hpp"

#include <string>
#include <functional>
#include <limits>

typedef std::function<void (const std::string &, int, int)> dir_callback_t;

ssize_t file_size(const std::string &source);
bool write_file(const std::string &source, unsigned char *buffer, ssize_t size);
bool read_file(const std::string &source, unsigned char *buffer, ssize_t size);
bool posix_transfer_file(const std::string &source, const std::string &dest, size_t soffset = 0, size_t doffset = 0, size_t size = std::numeric_limits<size_t>::max());

bool check_dir(const std::string &d);
bool parse_dir(const std::string &p, const std::string &cname, dir_callback_t f);

#endif //__FILE_UTIL
