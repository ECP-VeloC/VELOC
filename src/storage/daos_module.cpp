#include "daos_module.hpp"
#include "common/file_util.hpp"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

//#define __DEBUG
#include "common/debug.hpp"

daos_module_t::daos_module_t(const std::string &s, const std::string &p, const std::string &c) : scratch(s) {
    if (daos_init() != 0)
        FATAL("cannot initialize DAOS, aborting");
    int rc = daos_pool_connect(p.c_str(), NULL, DAOS_PC_RW, &poh, NULL, NULL);
    if (rc)
        FATAL("cannot connect to DAOS pool " << p << ", error code = " << rc);
    rc = daos_cont_open(poh, c.c_str(), DAOS_COO_RW, &coh, NULL, NULL);
    if (rc)
        FATAL("cannot open DAOS container " << c << ", error code = " << rc);
}

daos_obj_id_t daos_module_t::generate_kv_id(int unique_id) {
    daos_obj_id_t oid;
    oid.lo = unique_id;
    oid.hi = 0;
    daos_obj_generate_oid(coh, &oid, DAOS_OF_KV_FLAT, OC_SX, 0, 0);
    return oid;
}

void daos_module_t::get_versions(const command_t &cmd, std::set<int> &result) {
    daos_obj_id_t oid = generate_kv_id(cmd.unique_id);
    daos_handle_t oh;
    if (daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL))
        return;

    char buf[DAOS_BUFF_SIZE];
    daos_key_desc_t kds[DAOS_BATCH_SIZE];
    daos_anchor_t anchor = {0};
    d_sg_list_t sgl;
    d_iov_t sg_iov;

    d_iov_set(&sg_iov, buf, DAOS_BUFF_SIZE);
    sgl.sg_nr = 1;
    sgl.sg_nr_out = 0;
    sgl.sg_iovs = &sg_iov;

    std::regex e = command_t::regex(cmd.name);
    while (!daos_anchor_is_eof(&anchor)) {
        uint32_t nr = DAOS_BATCH_SIZE;
        if (daos_kv_list(oh, DAOS_TX_NONE, &nr, kds, &sgl, &anchor, NULL)) {
            daos_kv_close(oh, NULL);
            return;
        }
        char *ptr = buf;
        for (uint32_t i = 0; i < nr; ptr += kds[i].kd_key_len, i++) {
            std::string key(ptr, kds[i].kd_key_len);
            int id, v;
            DBG("found key: " << key);
            // no need to match id, each id has its own KV store
            if (command_t::match(key, e, id, v))
                result.insert(v);
        }
    }
    daos_kv_close(oh, NULL);
}

bool daos_module_t::flush(const command_t &cmd) {
    daos_obj_id_t oid = generate_kv_id(cmd.unique_id);
    daos_handle_t oh;

    TIMER_START(io_timer);
    int rc = daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL);
    if (rc) {
        ERROR("cannot open DAOS object id (" << oid.lo << ", " << oid.hi << "); error = " << rc);
        return false;
    }
    std::string source = cmd.filename(scratch);
    int fi = open(source.c_str(), O_RDONLY);
    ssize_t size = file_size(source);
    if (fi == -1 || size == -1) {
        ERROR("cannot open source " << source << "; error = " << std::strerror(errno));
        daos_kv_close(oh, NULL);
        return false;
    }
    unsigned char *buff = (unsigned char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fi, 0);
    if (buff == MAP_FAILED) {
        close(fi);
        daos_kv_close(oh, NULL);
        ERROR("cannot mmap source " << source << "; error = " << std::strerror(errno));
        return false;
    }
    close(fi);
    rc = daos_kv_put(oh, DAOS_TX_NONE, 0, cmd.stem().c_str(), size, buff, NULL);
    munmap(buff, size);
    daos_kv_close(oh, NULL);
    if (rc) {
        ERROR("daos_kv_put failed for " << source << "; error code = " << rc);
        return false;
    }
    TIMER_STOP(io_timer, "transferred " << source << " to DAOS object id (" << oid.lo << ", " << oid.hi << ")");
    return true;
}

bool daos_module_t::restore(const command_t &cmd) {
    daos_obj_id_t oid = generate_kv_id(cmd.unique_id);
    daos_handle_t oh;
    int rc = daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL);
    if (rc) {
        ERROR("cannot open DAOS object id (" << oid.lo << ", " << oid.hi << "); error = " << rc);
        return false;
    }
    size_t size;
    rc = daos_kv_get(oh, DAOS_TX_NONE, 0, cmd.stem().c_str(), &size, NULL, NULL);
    if (rc) {
        daos_kv_close(oh, NULL);
        ERROR("cannot get size of key " << cmd.stem() << "; error code = " << rc);
        return false;
    }
    std::string dest = cmd.filename(scratch);
    int fo = open(dest.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fo == -1) {
        ERROR("cannot open destination " << dest << "; error = " << std::strerror(errno));
        daos_kv_close(oh, NULL);
        return false;
    }
    if (posix_fallocate(fo, 0, size)) {
        close(fo);
        daos_kv_close(oh, NULL);
        ERROR("cannot preallocate " << dest << "; error = " << std::strerror(errno));
        return false;
    }
    unsigned char *buff = (unsigned char *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fo, 0);
    if (buff == MAP_FAILED) {
        close(fo);
        daos_kv_close(oh, NULL);
        ERROR("cannot mmap destination " << dest << "; error = " << std::strerror(errno));
        return false;
    }
    close(fo);
    rc = daos_kv_get(oh, DAOS_TX_NONE, 0, cmd.stem().c_str(), &size, buff, NULL);
    munmap(buff, size);
    daos_kv_close(oh, NULL);
    if (rc) {
        ERROR("daos_kv_get failed for " << dest << "; error code = " << rc);
        return false;
    }
    return true;
}

bool daos_module_t::exists(const command_t &cmd) {
    daos_obj_id_t oid = generate_kv_id(cmd.unique_id);
    daos_handle_t oh;
    int rc = daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL);
    if (rc) {
        ERROR("cannot open DAOS object id (" << oid.lo << ", " << oid.hi << "); error = " << rc);
        return false;
    }
    size_t size;
    rc = daos_kv_get(oh, DAOS_TX_NONE, 0, cmd.stem().c_str(), &size, NULL, NULL);
    daos_kv_close(oh, NULL);
    return rc == 0;
}

bool daos_module_t::remove(const command_t &cmd) {
    daos_obj_id_t oid = generate_kv_id(cmd.unique_id);
    daos_handle_t oh;
    int rc = daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL);
    if (rc) {
        ERROR("cannot open DAOS object id (" << oid.lo << ", " << oid.hi << "); error = " << rc);
        return false;
    }
    rc = daos_kv_remove(oh, DAOS_TX_NONE, 0, cmd.stem().c_str(), NULL);
    daos_kv_close(oh, NULL);
    return rc == 0;
}

daos_module_t::~daos_module_t() {
    daos_cont_close(coh, NULL);
    daos_pool_disconnect(poh, NULL);
    daos_fini();
}
