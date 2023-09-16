#ifndef __CKPT_UTIL
#define __CKPT_UTIL

#include <map>
#include <string>

size_t read_header(const std::string &ckpt_name, std::map<int, size_t> &region_info);

#endif // __CKPT_UTIL
