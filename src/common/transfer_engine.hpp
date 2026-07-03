#ifndef __TRANSFER_ENGINE_HPP
#define __TRANSFER_ENGINE_HPP

#include <cstddef>
#include <functional>
#include <memory>

class config_t;
struct xfer_group_impl_t;

// An endpoint is either a memory region (ptr) or an open file at an offset (fd).
// Whether a memory pointer is host or device memory is detected internally by
// the engine, so tiers are transparent to callers.
struct endpoint_t {
    void  *ptr = nullptr;
    int    fd  = -1;
    size_t off = 0;
};

inline endpoint_t mem(void *p) { return endpoint_t{p, -1, 0}; }
inline endpoint_t file(int fd, size_t off = 0) { return endpoint_t{nullptr, fd, off}; }

// A group bundles all transfers of one logical operation (a checkpoint or a
// restart) so they can be waited on together with two-phase completion. It is a
// lightweight handle sharing state with the engine's progress thread.
class xfer_group_t {
    std::shared_ptr<xfer_group_impl_t> g;
    friend class transfer_engine_t;
    explicit xfer_group_t(std::shared_ptr<xfer_group_impl_t> impl) : g(std::move(impl)) { }

public:
    xfer_group_t() = default; // empty group: waits succeed immediately

    // Queue a transfer of "len" bytes from "from" to "to". The engine inserts a
    // device<->host bounce through pinned staging when needed.
    void submit(endpoint_t from, endpoint_t to, size_t len);

    // Host memory owned by the group (freed when the group is destroyed). Used
    // for the header and serialized regions whose lifetime must outlive submit.
    void *alloc(size_t size);

    // A file descriptor closed by the engine once the group is fully durable.
    void adopt_fd(int fd);

    // Block until every source buffer has been read (application may resume).
    bool wait_sources();
    // Block until every transfer is durable in its destination.
    bool wait_completion();
    // Run "cont" on the progress thread once the group is durable (or now, if
    // it already is). Used to hand a completed local checkpoint to the backend.
    void on_completion(std::function<void()> cont);
};

class transfer_engine_t {
public:
    struct impl_t; // opaque, defined in transfer_engine.cpp

private:
    std::unique_ptr<impl_t> pimpl;
    explicit transfer_engine_t(const config_t &cfg);

public:
    ~transfer_engine_t();
    // Process-wide singleton (one engine, one progress thread per process).
    static transfer_engine_t &instance(const config_t &cfg);
    // Begin a new group of related transfers.
    xfer_group_t group();
};

#endif // __TRANSFER_ENGINE_HPP
