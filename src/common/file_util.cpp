#include "file_util.hpp"
#include "common/status.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>

#define __DEBUG
#include "debug.hpp"

bool write_file(const std::string &source, unsigned char *buffer, ssize_t size) {
    bool ret;
    int fd = open(source.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (fd == -1) {
        ERROR("cannot open " << source << ", error = " << strerror(errno));
        return false;
    }
    ret = write(fd, buffer, size) == size;
    if (!ret)
        ERROR("cannot write " << size << " bytes to " << source << " , error = " << std::strerror(errno));
    close(fd);
    return ret;
}

bool read_file(const std::string &source, unsigned char *buffer, ssize_t size) {
    bool ret;
    int fd = open(source.c_str(), O_RDONLY);
    if (fd == -1) {
        ERROR("cannot open " << source << ", error = " << std::strerror(errno));
        return false;
    }
    ret = read(fd, buffer, size) == size;
    if (!ret)
        ERROR("cannot read " << size << " bytes from " << source << " , error = " << std::strerror(errno));
    close(fd);
    return ret;
}

bool posix_transfer_file(const std::string &source, const std::string &dest) {
    TIMER_START(io_timer);
    int fi = open(source.c_str(), O_RDONLY);
    if (fi == -1) {
	ERROR("cannot open source " << source << "; error = " << std::strerror(errno));
	return false;
    }
    int fo = open(dest.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fo == -1) {
	close(fi);
	ERROR("cannot open destination " << dest << "; error = " << std::strerror(errno));
	return false;
    }
    struct stat st;
    stat(source.c_str(), &st);
    size_t remaining = st.st_size;
    while (remaining > 0) {
	ssize_t transferred = sendfile(fo, fi, NULL, remaining);
	if (transferred == -1) {
	    close(fi);
	    close(fo);
	    ERROR("cannot copy " <<  source << " to " << dest << "; error = " << std::strerror(errno));
	    return false;
	} else
	    remaining -= transferred;
    }
    close(fi);
    close(fo);
    TIMER_STOP(io_timer, "transferred " << source << " to " << dest);
    return true;
}

bool dir_exists(const std::string &dir) {
    DIR *entry = opendir(dir.c_str());
    if (entry == NULL)
        return false;
    closedir(entry);
    return true;
}

int get_latest_version(const std::string &p, const std::string &cname, int needed_version) {
    struct dirent *dentry;
    DIR *dir;
    int version, ret = VELOC_IGNORED;

    dir = opendir(p.c_str());
    if (dir == NULL)
	return -1;
    while ((dentry = readdir(dir)) != NULL) {
	std::string fname = std::string(dentry->d_name);
	if (fname.compare(0, cname.length(), cname) == 0 &&
	    sscanf(fname.substr(cname.length()).c_str(), "-%d", &version) == 1 &&
	    (needed_version == 0 || version <= needed_version) &&
	    access((p + "/" + fname).c_str(), R_OK) == 0) {
	    if (version > ret)
		ret = version;
	}
    }
    closedir(dir);
    return ret;
}
