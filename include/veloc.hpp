#ifndef __VELOC_HPP
#define __VELOC_HPP

#include "veloc/defs.h"

#include <mpi.h>
#include <string>
#include <iostream>
#include <set>
#include <map>
#include <functional>

namespace veloc {

class client_t {
protected:
    static constexpr char default_mem_group[] = "default";
    using serializer_t = std::function<void (std::ostream &)>;
    using deserializer_t = std::function<bool (std::istream &)>;
    using observer_t = std::function<void (const std::string &name, int version)>;

public:
    virtual bool mem_protect(int id, void *ptr, size_t count, size_t base_size, const std::string &name = default_mem_group) = 0;
    virtual bool mem_protect(int id, const serializer_t &s, const deserializer_t &d, const std::string &name = default_mem_group) = 0;
    virtual bool mem_unprotect(int id, const std::string &name = default_mem_group) = 0;
    virtual void mem_clear(const std::string &name = default_mem_group) = 0;
    virtual bool register_observer(int type, const observer_t &obs) = 0;

    virtual std::string route_file(const std::string &original) = 0;
    virtual bool cleanup(const std::string &name) = 0;

    virtual bool checkpoint(const std::string &name, int version) = 0;
    virtual bool checkpoint_begin(const std::string &name, int version) = 0;
    virtual bool checkpoint_mem(int mode, const std::set<int> &ids) = 0;
    virtual bool checkpoint_end(bool success) = 0;
    virtual bool checkpoint_wait() = 0;
    virtual bool checkpoint_finished() = 0;

    virtual int restart_test(const std::string &name, int version, int target_rank = -1) = 0;
    virtual bool restart(const std::string &name, int version, int target_rank = -1) = 0;
    virtual bool restart_begin(const std::string &name, int version, int target_rank = -1) = 0;
    virtual size_t recover_size(int id) = 0;
    virtual bool recover_mem(int mode, const std::set<int> &ids) = 0;
    virtual bool restart_end(bool success) = 0;

    virtual ~client_t() = 0;
};

client_t *get_client(unsigned int id, const std::string &cfg_file);
client_t *get_client(MPI_Comm comm, const std::string &cfg_file);

}

#endif // __VELOC_HPP
