#include "include/veloc.h"
#include "client.hpp"

static veloc_client_t *veloc_client = NULL;

#define __DEBUG
#include "common/debug.hpp"

void __attribute__ ((constructor)) veloc_constructor() {
}

void __attribute__ ((destructor)) veloc_destructor() {
}

extern "C" int VELOC_Init(MPI_Comm comm, const char *cfg_file) {
    try {
	veloc_client = new veloc_client_t(comm, cfg_file);
	return VELOC_SUCCESS;
    } catch (std::exception &e) {
	ERROR(e.what());
	return VELOC_FAILURE;
    }
}

#define CLIENT_CALL(x) (veloc_client != NULL && (x)) ? VELOC_SUCCESS : VELOC_FAILURE;

extern "C" int VELOC_Mem_protect(int id, void *ptr, size_t count, size_t base_size) {
    return CLIENT_CALL(veloc_client->mem_protect(id, ptr, count, base_size));
}

extern "C" int VELOC_Mem_unprotect(int id) {
    return CLIENT_CALL(veloc_client->mem_unprotect(id));
}

extern "C" int VELOC_Checkpoint_begin(const char *name, int version) {
    return CLIENT_CALL(veloc_client->checkpoint_begin(name, version));
}

extern "C" int VELOC_Checkpoint_mem() {
    return CLIENT_CALL(veloc_client->checkpoint_mem());
}

extern "C" int VELOC_Checkpoint_end(int success) {
    return CLIENT_CALL(veloc_client->checkpoint_end(success));
}

extern "C" int VELOC_Checkpoint_wait() {
    return CLIENT_CALL(veloc_client->checkpoint_wait());
}

extern "C" int VELOC_Restart_test(const char *name, int version) {
    if (veloc_client == NULL)
	return -1;
    return veloc_client->restart_test(name, version);
}

extern "C" int VELOC_Route_file(char *ckpt_file_name) {
    std::string cname = veloc_client->route_file();
    cname.copy(ckpt_file_name, cname.length());
    ckpt_file_name[cname.length()] = 0;
    return VELOC_SUCCESS;
}

extern "C" int VELOC_Restart_begin(const char *name, int version) {
    return CLIENT_CALL(veloc_client->restart_begin(name, version));
}

extern "C" int VELOC_Recover_mem() {
    std::set<int> ids = {};
    return CLIENT_CALL(veloc_client->recover_mem(VELOC_RECOVER_ALL, ids));
}

extern "C" int VELOC_Restart_end(int success) {
    return CLIENT_CALL(veloc_client->restart_end(success));
}

extern "C" int VELOC_Restart(const char *name, int version) {
    int ret = VELOC_Restart_begin(name, version);
    if (ret == VELOC_SUCCESS)
	ret = VELOC_Recover_mem();
    if (ret == VELOC_SUCCESS)
	ret = VELOC_Restart_end(1);
    return ret;
}

extern "C" int VELOC_Checkpoint(const char *name, int version) {
    int ret = VELOC_Checkpoint_wait();
    if (ret == VELOC_SUCCESS)
	VELOC_Checkpoint_begin(name, version);
    if (ret == VELOC_SUCCESS)
	ret = VELOC_Checkpoint_mem();
    if (ret == VELOC_SUCCESS)
	ret = VELOC_Checkpoint_end(1);
    return ret;
}

extern "C" int VELOC_Finalize(int cleanup) {
    if (veloc_client != NULL) {
	if (cleanup)
	    veloc_client->cleanup();
	delete veloc_client;
	return VELOC_SUCCESS;
    } else {
	ERROR("Attempting to finalize VELOC before it was initialized");
	return VELOC_FAILURE;
    }
}
