#include "client.hpp"

static veloc::client_t *veloc_client = NULL;

#define __DEBUG
#include "common/debug.hpp"

void __attribute__ ((constructor)) veloc_constructor() {
}

void __attribute__ ((destructor)) veloc_destructor() {
}

// C interface

extern "C" int VELOC_Init(MPI_Comm comm, const char *cfg_file) {
    try {
	veloc_client = veloc::get_client(comm, cfg_file);
	return VELOC_SUCCESS;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
	return VELOC_FAILURE;
    }
}

extern "C" int VELOC_Init_single(unsigned int id, const char *cfg_file) {
    try {
	veloc_client = veloc::get_client(id, cfg_file);
	return VELOC_SUCCESS;
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
	return VELOC_FAILURE;
    }
}

#define BOOL_CALL(x) (veloc_client != NULL && (x)) ? VELOC_SUCCESS : VELOC_FAILURE;
#define VOID_CALL(x) veloc_client != NULL ? (x, VELOC_SUCCESS) : VELOC_FAILURE;
#define INT_CALL(x) veloc_client != NULL ? x : VELOC_FAILURE;

extern "C" int VELOC_Mem_protect(int id, void *ptr, size_t count, size_t base_size) {
    return BOOL_CALL(veloc_client->mem_protect(id, ptr, count, base_size));
}

extern "C" int VELOC_Region_protect(int id, void *ptr, size_t count, size_t base_size, const char *name) {
    return BOOL_CALL(veloc_client->mem_protect(id, ptr, count, base_size, name));
}

extern "C" int VELOC_Mem_unprotect(int id) {
    return BOOL_CALL(veloc_client->mem_unprotect(id));
}

extern "C" int VELOC_Region_unprotect(int id, const char *name) {
    return BOOL_CALL(veloc_client->mem_unprotect(id, name));
}

extern "C" int VELOC_Mem_clear() {
    return VOID_CALL(veloc_client->mem_clear());
}

extern "C" int VELOC_Region_clear(const char *name) {
    return VOID_CALL(veloc_client->mem_clear(name));
}

extern "C" int VELOC_Checkpoint_begin(const char *name, int version) {
    return BOOL_CALL(veloc_client->checkpoint_begin(name, version));
}

extern "C" int VELOC_Checkpoint_mem() {
    return BOOL_CALL(veloc_client->checkpoint_mem(VELOC_CKPT_ALL, {}));
}

extern "C" int VELOC_Checkpoint_selective(int mode, int *ids, int no_ids) {
    std::set<int> id_set = {};
    for (int i = 0; i < no_ids; i++)
	id_set.insert(ids[i]);
    return BOOL_CALL(veloc_client->checkpoint_mem(mode, id_set));
}

extern "C" int VELOC_Checkpoint_end(int success) {
    return BOOL_CALL(veloc_client->checkpoint_end(success));
}

extern "C" int VELOC_Checkpoint_wait() {
    return BOOL_CALL(veloc_client->checkpoint_wait());
}

extern "C" int VELOC_Checkpoint_finished() {
    return BOOL_CALL(veloc_client->checkpoint_finished());
}

extern "C" int VELOC_Restart_test(const char *name, int version) {
    return INT_CALL(veloc_client->restart_test(name, version));
}

extern "C" int VELOC_Route_file(const char *original, char *routed) {
    std::string cname = veloc_client->route_file(original);
    cname.copy(routed, cname.length());
    routed[cname.length()] = 0;

    return routed[0] != 0 ? VELOC_SUCCESS : VELOC_FAILURE;
}

extern "C" int VELOC_Cleanup(const char *name) {
    return BOOL_CALL(veloc_client->cleanup(std::string(name)));
}

extern "C" int VELOC_Restart_begin(const char *name, int version) {
    return BOOL_CALL(veloc_client->restart_begin(std::string(name), version));
}

extern "C" int VELOC_Recover_mem() {
    return BOOL_CALL(veloc_client->recover_mem(VELOC_RECOVER_ALL, {}));
}

extern "C" int VELOC_Recover_selective(int mode, int *ids, int no_ids) {
    std::set<int> id_set = {};
    for (int i = 0; i < no_ids; i++)
	id_set.insert(ids[i]);
    return BOOL_CALL(veloc_client->recover_mem(mode, id_set));
}

extern "C" int VELOC_Recover_size(int id) {
    return INT_CALL(veloc_client->recover_size(id));
}

extern "C" int VELOC_Restart_end(int success) {
    return BOOL_CALL(veloc_client->restart_end(success));
}

extern "C" int VELOC_Restart(const char *name, int version) {
    return BOOL_CALL(veloc_client->restart(name, version));
}

extern "C" int VELOC_Checkpoint(const char *name, int version) {
    return BOOL_CALL(veloc_client->checkpoint(name, version));
}

extern "C" int VELOC_Finalize(int drain) {
    if (veloc_client != NULL) {
        int ret = VELOC_SUCCESS;
	if (drain)
	    ret = VELOC_Checkpoint_wait();
	delete veloc_client;
	return ret;
    } else {
	ERROR("Attempting to finalize VELOC before it was initialized");
	return VELOC_FAILURE;
    }
}

// C++ interface

veloc::client_t *veloc::get_client(MPI_Comm comm, const std::string &cfg_file) {
    if (veloc_client == NULL)
        veloc_client = new client_impl_t(comm, cfg_file);
    return veloc_client;
}

veloc::client_t *veloc::get_client(unsigned int id, const std::string &cfg_file) {
    if (veloc_client == NULL)
        veloc_client = new client_impl_t(id, cfg_file);
    return veloc_client;
}

veloc::client_t::~client_t() {
}
