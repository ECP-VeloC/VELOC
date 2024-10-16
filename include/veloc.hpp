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
    typedef std::function<void (std::ostream &)> serializer_t;
    typedef std::function<bool (std::istream &)> deserializer_t;
    typedef std::function<void (const std::string &name, int version)> observer_t;
    struct region_t {
        void *ptr;
        size_t size;
        serializer_t s;
        deserializer_t d;
        region_t(void *p, size_t s) : ptr(p), size(s) { }
        region_t(const serializer_t &_s, const deserializer_t &_d) : ptr(NULL), s(_s), d(_d) { }
    };
    typedef std::map<int, region_t> regions_t;
    typedef std::map<int, observer_t> observers_t;

    regions_t mem_regions;
    observers_t observers;
public:
    virtual bool mem_protect(int id, void *ptr, size_t count, size_t base_size) {
        return mem_regions.insert_or_assign(id, region_t(ptr, count * base_size)).second;
    }
    virtual bool mem_protect(int id, const serializer_t &s, const deserializer_t &d) {
        return mem_regions.insert_or_assign(id, region_t(s, d)).second;
    }
    virtual bool mem_unprotect(int id) {
        return mem_regions.erase(id) > 0;
    }
    virtual void mem_clear() {
	mem_regions.clear();
    }
    virtual bool register_observer(int type, const observer_t &obs) {
	return observers.insert_or_assign(type, obs).second;
    }

    virtual std::string route_file(const std::string &original) = 0;
    virtual bool cleanup(const std::string &name) = 0;

    virtual bool checkpoint(const std::string &name, int version) = 0;
    virtual bool checkpoint_begin(const std::string &name, int version) = 0;
    virtual bool checkpoint_mem(int mode, const std::set<int> &ids) = 0;
    virtual bool checkpoint_end(bool success) = 0;
    virtual bool checkpoint_wait() = 0;
    virtual bool checkpoint_finished() = 0;

    virtual int restart_test(const std::string &name, int version) = 0;
    virtual bool restart(const std::string &name, int version) = 0;
    virtual bool restart_begin(const std::string &name, int version) = 0;
    virtual size_t recover_size(int id) = 0;
    virtual bool recover_mem(int mode, const std::set<int> &ids) = 0;
    virtual bool restart_end(bool success) = 0;

    virtual ~client_t() = 0;
};

client_t *get_client(unsigned int id, const std::string &cfg_file);
client_t *get_client(MPI_Comm comm, const std::string &cfg_file);

}

#endif // __VELOC_HPP
