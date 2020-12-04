#include "daos_module.hpp"
#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

static bool daos_transfer_file(const command_t &c) {
    daos_obj_id_t oid;
    daos_handle_t oh;

    TIMER_START(io_timer);
    oid.lo = c.unique_id;
    oid.hi = 0;
    daos_obj_generate_id(&oid, DAOS_OF_KV_FLAT, OC_SX, 0);
    int ret = daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL);
    if (ret != 0) {
        ERROR("cannot open DAOS object id (" << oid.lo << ", " << oid.hi << "); error = " << ret);
        return false;
    }
    int fi = open(c.filename(cfg.get("scratch")).c_str(), O_RDONLY);
    if (fi == -1) {
	ERROR("cannot open source " << source << "; error = " << std::strerror(errno));
	return false;
    }
    size_t size = lseek(fd, 0, SEEK_END);
    unsigned char *buff = (unsigned char *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fi, 0);
    close(fi);
    ret = daos_kv_put(oh, DAOS_TX_NONE, 0, c.stem().c_str(), size, buff, NULL);
    munmap(buff, size);
    daos_kv_close(oh, NULL);
    if (ret == 0) {
        TIMER_STOP(io_timer, "transferred " << source << " to DAOS object id (" << oid.lo << ", " << oid.hi << ")");
        return true;
    } else {
        ERROR("cannot copy " <<  source << " to DAOS object id (" << oid.lo << ", " << oid.hi << "); error = " << ret);
        return false;
    }
}

daos_module_t::daos_module_t(const config_t &c) : cfg(c) {
    std::string uuid_name;
    uuid_t pool_id, cont_id;

    if (!cfg.get_optional("daos_pool_uuid", &uuid_name)) {
        interval = -1;
        return;
    }
    INFO("requested to use DAOS, pool_uuid = " << uuid_name);
    if (uuid_parse(uuid_name.c_str(), pool_id) != 0)
        FATAL("cannot parse daos_pool_uuid (config file), please check format");
    uuid_name = cfg.get("daos_cont_uuid");
    if (uuid_parse(uuid_name.c_str(), cont_id) != 0)
        FATAL("cannot parse daos_cont_uuid (config file), please check format");

    if (daos_init() != 0)
        FATAL("cannot initialize DAOS");
    if (daos_pool_connect(pool_id, NULL, NULL, DAOS_PC_RW, &poh, NULL, NULL) != 0)
        FATAL("cannot connect to DAOS pool");
    if (daos_cont_create(poh, cont_id, NULL, NULL) != 0)
        FATAL("cannot create container: " << uuid_name);
    if (daos_cont_open(poh, cont_id, DAOS_COO_RW, &coh, NULL, NULL) != 0)
        FATAL("cannot open container: " << uuid_name);
    if (!cfg.get_optional("daos_interval", interval)) {
	INFO("daos_interval not specified, every checkpoint will be persisted");
	interval = 0;
    }
}

daos_module_t::~daos_module_t() {
    daos_cont_close(coh, NULL);
    daos_pool_disconnect(poh, NULL);
    daos_fini();
}

int transfer_module_t::process_command(const command_t &c) {
    if (interval < 0)
        return VELOC_IGNORED;

    switch (c.command) {
    case command_t::INIT:
	last_timestamp[c.unique_id] = std::chrono::system_clock::now() + std::chrono::seconds(interval);
	return VELOC_SUCCESS;

    case command_t::CHECKPOINT:
	if (interval > 0) {
	    auto t = std::chrono::system_clock::now();
	    if (t < last_timestamp[c.unique_id])
		return VELOC_SUCCESS;
	    else
		last_timestamp[c.unique_id] = t + std::chrono::seconds(interval);
	}
        return daos_transfer_file(c) ? VELOC_SUCCESS : VELOC_FAILURE;

    case command_t::RESTART:
        ERROR("restart not yet implemeted for DAOS");
        return VELOC_IGNORED;

    default:
	return VELOC_IGNORED;
    }
}
