#include "file_util.hpp"
#include "command.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0) || (__GLIBC__ >= 2 && __GLIBC_MINOR__ >= 27)
    #define COPY_FILE_RANGE_AVAIL
#else
    #include <sys/sendfile.h>
#endif

#include <cerrno>
#include <cstring>

//#define __DEBUG
#include "debug.hpp"

bool parse_dir(const std::string &p, const std::string &cname, dir_callback_t f) {
    DIR *dir;
    dir = opendir(p.c_str());
    if (dir == NULL)
	return false;
    std::regex e = command_t::regex(cname);
    dirent *dentry;
    while ((dentry = readdir(dir)) != NULL) {
        if (dentry->d_type == DT_REG || dentry->d_type == DT_LNK) {
            std::string dname(dentry->d_name);
            int rank, version;
            if (command_t::match(dname, e, rank, version))
                f(p + "/" + dentry->d_name, rank, version);
        }
    }
    closedir(dir);

    return true;
}

ssize_t file_size(const std::string &source) {
    struct stat stat_buf;
    int rc = stat(source.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

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

#ifdef WITH_POSIX_DIRECT
bool file_transfer_loop(int fs, size_t soff, int fd, size_t doff, size_t remaining) {
    bool success = true;
    while (remaining > 0) {
#if COPY_FILE_RANGE_AVAIL
	ssize_t transferred = copy_file_range(fs, (off64_t *)&soff, fd, (off64_t *)&doff, remaining, 0);
#else
        //Some bookkeping to maintain copy_file_range semantics.
        off64_t fd_offset_prev = lseek(fd, 0, SEEK_CUR);
        lseek(fd, (off64_t)doff, SEEK_SET);
        ssize_t transferred = sendfile(fd, fs, (off64_t*)&soff, remaining);
        lseek(fs, fd_offset_prev, SEEK_SET);
#endif
        if (transferred == -1) {
            success = false;
            break;
        }
        remaining -= transferred;
    }
    return success;
}
#elif WITH_POSIX_RW
bool file_transfer_loop(int fs, size_t soff, int fd, size_t doff, size_t remaining) {
    const size_t MAX_BUFF_SIZE = 1 << 24;
    bool success = true;
    char *buff = new char[MAX_BUFF_SIZE];
    while (remaining > 0) {
	ssize_t transferred = pread(fs, buff, std::min(MAX_BUFF_SIZE, (size_t)remaining), soff);
        if (transferred == -1 || pwrite(fd, buff, transferred, doff) != transferred) {
            success = false;
            break;
        }
        remaining -= transferred;
	soff += transferred;
	doff += transferred;
    }
    delete []buff;
    return success;
}
#else
#error Invalid POSIX IO transfer method selected. Valid choices: WITH_POSIX_DIRECT, WITH_POSIX_RW
#endif

bool posix_transfer_file(const std::string &source, const std::string &dest, size_t soffset, size_t doffset, size_t size) {
    TIMER_START(io_timer);
    int fs = open(source.c_str(), O_RDONLY);
    if (fs == -1) {
	ERROR("cannot open source " << source << "; error = " << std::strerror(errno));
	return false;
    }
    int fd = open(dest.c_str(), O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
	close(fs);
	ERROR("cannot open destination " << dest << "; error = " << std::strerror(errno));
	return false;
    }
    ssize_t remaining = std::min(size, file_size(source.c_str()) - soffset);
    bool success = file_transfer_loop(fs, soffset, fd, doffset, remaining);
    close(fs);
    close(fd);
    if (success) {
        TIMER_STOP(io_timer, "transferred " << source << " to " << dest);
    } else
        ERROR("cannot copy " <<  source << " to " << dest << "; error = " << std::strerror(errno));
    return success;
}

bool check_dir(const std::string &d) {
    mkdir(d.c_str(), 0755);
    DIR *entry = opendir(d.c_str());
    if (entry == NULL)
        return false;
    closedir(entry);
    return true;
}

std::string unique_suffix() {
    char host_name[HOST_NAME_MAX] = "";
    gethostname(host_name, HOST_NAME_MAX);
    return std::string(host_name) + "-" + std::to_string(getuid());
}
