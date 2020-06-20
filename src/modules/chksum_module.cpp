#include "chksum_module.hpp"
#include "common/file_util.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <openssl/md5.h>

#define __DEBUG
#include "common/debug.hpp"

chksum_module_t::chksum_module_t(const config_t &c) : cfg(c) {
    active = cfg.get_optional("chksum", false);
    if (active && !check_dir(cfg.get("meta"))) {
        ERROR("metadata directory " << cfg.get("meta") << " inaccessible, checksumming deactivated!");
        active = false;
    }
    INFO("checksumming active: " << active);
}

static bool chksum_file(const std::string &source, unsigned char *result) {
    int fd = open(source.c_str(), O_RDONLY);
    if (fd == -1) {
        ERROR("cannot open " << source << ", error = " << std::strerror(errno));
        return false;
    }
    size_t size = lseek(fd, 0, SEEK_END);
    unsigned char *buff = (unsigned char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (buff == MAP_FAILED) {
        ERROR("cannot mmap " << source << ", error = " << std::strerror(errno));
	return false;
    }
    MD5(buff, size, result);
    munmap(buff, size);
    return true;
}

int chksum_module_t::process_command(const command_t &c) {
    if (!active)
        return VELOC_IGNORED;

    const unsigned int HASH_SIZE = 16;
    unsigned char chksum[HASH_SIZE], orig_chksum[HASH_SIZE];
    std::string meta = c.filename(cfg.get("meta")) + ".md5",
        local = c.filename(cfg.get("scratch"));
    bool match;

    switch (c.command) {
    case command_t::CHECKPOINT:
        if (!chksum_file(local, chksum))
            return VELOC_FAILURE;
        return write_file(meta, chksum, HASH_SIZE) ? VELOC_SUCCESS : VELOC_FAILURE;

    case command_t::RESTART:
        if (!read_file(meta, orig_chksum, HASH_SIZE))
            return VELOC_FAILURE;
        if (!chksum_file(local, chksum))
            return VELOC_FAILURE;
        match = memcmp(chksum, orig_chksum, HASH_SIZE) == 0;
        if (match)
            return VELOC_SUCCESS;
        else {
            ERROR("checksum of file " << local << " does not match records, restart will fail");
            return VELOC_FAILURE;
        }

    default:
        return VELOC_IGNORED;
    }
}
