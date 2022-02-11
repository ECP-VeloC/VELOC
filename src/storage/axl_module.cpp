#include "axl_module.hpp"

#include <map>
#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

static const std::map<std::string, axl_xfer_t> axl_type_strs = {
    {"default", AXL_XFER_DEFAULT},
    {"native", AXL_XFER_NATIVE},
    {"AXL_XFER_SYNC", AXL_XFER_SYNC},
    {"AXL_XFER_ASYNC_DW", AXL_XFER_ASYNC_DW},
    {"AXL_XFER_ASYNC_BBAPI", AXL_XFER_ASYNC_BBAPI}
};

axl_module_t::axl_module_t(const std::string &s, const std::string &p, const std::string &axl_type_str) : posix_module_t(s, p) {
    auto e = axl_type_strs.find(axl_type_str);
    if (e == axl_type_strs.end())
        FATAL("AXL has no transfer type called \"" << axl_type_str <<"\", please consult the documentation");
    axl_type = e->second;
    int ret = AXL_Init();
    if (ret)
        FATAL("AXL initialization failed, error code: " << ret);
}

bool axl_module_t::axl_transfer_file(const std::string &source, const std::string &dest) {
    int id = AXL_Create(axl_type, source.c_str(), NULL), result = id;
    if (result < 0)
        goto err;
    if ((result = AXL_Add(id, (char *)source.c_str(), (char *)dest.c_str())))
        goto err;
    if ((result = AXL_Dispatch(id)))
        goto err;
    if ((result = AXL_Wait(id)))
        goto err;
    if ((result = AXL_Free(id)))
        goto err;
    return true;
  err:
    ERROR("AXL transfer from " << source << " to " << dest << " failed, error code: " << result);
    return false;
}

bool axl_module_t::flush(const command_t &cmd) {
    // memory-based API
    if (cmd.original[0] == 0)
        return axl_transfer_file(cmd.filename(scratch), cmd.filename(persistent));
    // file-based API
    if (!axl_transfer_file(cmd.filename(scratch), cmd.original))
        return false;
    unlink(cmd.filename(persistent).c_str());
    if (symlink(cmd.original, cmd.filename(persistent).c_str())) {
        ERROR("cannot create symlink " << cmd.filename(persistent) << " pointing at " << cmd.original << ", error: " << std::strerror(errno));
        return false;
    }
    return true;
}

bool axl_module_t::restore(const command_t &cmd) {
    return axl_transfer_file(cmd.filename(persistent), cmd.filename(scratch));
}

axl_module_t::~axl_module_t() {
    AXL_Finalize();
}
