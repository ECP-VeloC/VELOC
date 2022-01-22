#ifndef __DAOS_MODULE_HPP
#define __DAOS_MODULE_HPP

#include "storage_module.hpp"
#include <daos.h>

class daos_module_t : public storage_module_t {
    std::string scratch;
    static const size_t DAOS_BATCH_SIZE = 1024,
        DAOS_BUFF_SIZE = DAOS_BATCH_SIZE * command_t::CKPT_NAME_MAX;
    daos_handle_t poh, coh;
    daos_obj_id_t generate_kv_id(int unique_id);

public:
    daos_module_t(const std::string &scratch, const std::string &pool, const std::string &cont);
    virtual ~daos_module_t();
    virtual void get_versions(const command_t &cmd, std::set<int> &result);
    virtual bool flush(const command_t &cmd);
    virtual bool restore(const command_t &cmd);
    virtual bool remove(const command_t &cmd);
    virtual bool exists(const command_t &cmd);
};

#endif //__DAOS_MODULE_HPP
