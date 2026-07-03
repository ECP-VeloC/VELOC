#include "transfer_engine.hpp"
#include "file_provider.hpp"
#include "common/gpu/gpu_provider.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <algorithm>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

// Compile-time tunables (overridable via -D).
#ifndef VELOC_XFER_CHUNK
#define VELOC_XFER_CHUNK (8UL << 20)   // 8 MiB per staged chunk
#endif
#ifndef VELOC_GPU_STAGE_SLOTS
#define VELOC_GPU_STAGE_SLOTS 8        // bounded pinned pool: SLOTS * CHUNK bytes
#endif

// Fallback factory when no GPU support is compiled in: everything is host memory.
#ifndef WITH_CUDA
gpu_provider_t *create_gpu_provider() { return nullptr; }
#endif

namespace {
enum kind_t { K_HOST, K_DEVICE, K_FILE };
struct dchunk_t { void *dev; int fd; size_t foff; size_t n; };
}

struct xfer_t { endpoint_t from, to; size_t len; };

struct xfer_group_impl_t {
    transfer_engine_t::impl_t *eng = nullptr;
    std::vector<xfer_t> transfers;
    std::vector<std::unique_ptr<char[]>> buffers; // header/serialized data, alive until group destroyed
    std::vector<int> fds;                         // closed by the engine when durable

    size_t src_total = 0, src_done = 0;
    size_t all_total = 0, all_done = 0;
    bool queued = false, work_done = false, completion_ran = false, failed = false;
    std::function<void()> completion;

    ~xfer_group_impl_t() {
        for (int fd : fds)
            if (fd >= 0)
                ::close(fd);
    }
};

struct transfer_engine_t::impl_t {
    gpu_provider_t *gpu = nullptr;
    size_t chunk = VELOC_XFER_CHUNK;
    std::vector<void *> slots_all;   // every allocated slot (for teardown)
    std::vector<bool> slots_pinned;
    std::vector<void *> free_slots;  // touched only by the worker thread

    std::mutex mtx;
    std::condition_variable app_cv;  // application waiters (wait_sources/wait_completion)
    std::condition_variable work_cv; // progress thread
    std::deque<std::shared_ptr<xfer_group_impl_t>> ready;
    std::thread worker;
    bool stop = false;

    impl_t() {
        gpu = create_gpu_provider();
        if (gpu != nullptr) {
            for (int i = 0; i < VELOC_GPU_STAGE_SLOTS; i++) {
                void *s = gpu->alloc_pinned(chunk);
                bool pinned = (s != nullptr);
                if (s == nullptr)
                    s = ::malloc(chunk);
                if (s == nullptr)
                    break;
                slots_all.push_back(s);
                slots_pinned.push_back(pinned);
                free_slots.push_back(s);
            }
            DBG("GPU staging pool: " << slots_all.size() << " slots of " << chunk << " bytes");
        }
        worker = std::thread([this] { run(); });
    }

    ~impl_t() {
        {
            std::unique_lock<std::mutex> lk(mtx);
            stop = true;
            work_cv.notify_all();
        }
        if (worker.joinable())
            worker.join();
        for (size_t i = 0; i < slots_all.size(); i++) {
            if (slots_pinned[i])
                gpu->free_pinned(slots_all[i]);
            else
                ::free(slots_all[i]);
        }
        delete gpu;
    }

    int classify(const endpoint_t &e) {
        if (e.fd >= 0)
            return K_FILE;
        if (gpu != nullptr && gpu->is_device(e.ptr))
            return K_DEVICE;
        return K_HOST;
    }

    void ensure_queued_locked(const std::shared_ptr<xfer_group_impl_t> &g) {
        if (!g->queued) {
            g->queued = true;
            ready.push_back(g);
            work_cv.notify_one();
        }
    }

    void add_progress(const std::shared_ptr<xfer_group_impl_t> &g, size_t dsrc, size_t dall) {
        std::unique_lock<std::mutex> lk(mtx);
        g->src_done += dsrc;
        g->all_done += dall;
        app_cv.notify_all();
    }

    void fail(const std::shared_ptr<xfer_group_impl_t> &g) {
        std::unique_lock<std::mutex> lk(mtx);
        g->failed = true;
    }

    void run() {
        while (true) {
            std::shared_ptr<xfer_group_impl_t> g;
            {
                std::unique_lock<std::mutex> lk(mtx);
                work_cv.wait(lk, [&] { return stop || !ready.empty(); });
                if (ready.empty()) {
                    if (stop)
                        break;
                    continue;
                }
                g = ready.front();
                ready.pop_front();
            }
            process(g);
        }
    }

    void run_device_out(const std::shared_ptr<xfer_group_impl_t> &g, std::vector<dchunk_t> &chunks) {
        // Greedy device->host staging: drain all D2H copies (releasing the
        // application's device memory) as fast as the bounded pool allows; the
        // host->file writes proceed in the background afterwards.
        std::deque<std::pair<void *, dchunk_t>> staged;
        for (auto &c : chunks) {
            if (g->failed)
                break;
            while (free_slots.empty()) {
                auto st = staged.front();
                staged.pop_front();
                if (!file_provider::write_range(st.second.fd, st.second.foff, st.first, st.second.n))
                    fail(g);
                else
                    add_progress(g, 0, st.second.n);
                free_slots.push_back(st.first);
            }
            void *slot = free_slots.back();
            free_slots.pop_back();
            if (!gpu->copy_d2h(slot, c.dev, c.n)) {
                fail(g);
                free_slots.push_back(slot);
                break;
            }
            add_progress(g, c.n, 0); // device source released
            staged.emplace_back(slot, c);
        }
        while (!staged.empty()) {
            auto st = staged.front();
            staged.pop_front();
            if (!g->failed) {
                if (!file_provider::write_range(st.second.fd, st.second.foff, st.first, st.second.n))
                    fail(g);
                else
                    add_progress(g, 0, st.second.n);
            }
            free_slots.push_back(st.first);
        }
    }

    void process(const std::shared_ptr<xfer_group_impl_t> &g) {
        std::vector<dchunk_t> dev_out;
        for (auto &t : g->transfers) {
            if (g->failed)
                break;
            int kf = classify(t.from), kt = classify(t.to);
            if (kf == K_HOST && kt == K_FILE) {
                if (!file_provider::write_range(t.to.fd, t.to.off, t.from.ptr, t.len))
                    fail(g);
                else
                    add_progress(g, t.len, t.len);
            } else if (kf == K_DEVICE && kt == K_FILE) {
                for (size_t o = 0; o < t.len; o += chunk) {
                    size_t n = std::min(chunk, t.len - o);
                    dev_out.push_back({(char *)t.from.ptr + o, t.to.fd, t.to.off + o, n});
                }
            } else if (kf == K_FILE && kt == K_HOST) {
                if (!file_provider::read_range(t.from.fd, t.from.off, t.to.ptr, t.len))
                    fail(g);
                else
                    add_progress(g, t.len, t.len);
            } else if (kf == K_FILE && kt == K_DEVICE) {
                for (size_t o = 0; o < t.len && !g->failed; o += chunk) {
                    size_t n = std::min(chunk, t.len - o);
                    void *slot = free_slots.empty() ? nullptr : free_slots.back();
                    if (slot == nullptr) {
                        ERROR("no staging slot available for device restore");
                        fail(g);
                        break;
                    }
                    free_slots.pop_back();
                    if (!file_provider::read_range(t.from.fd, t.from.off + o, slot, n) ||
                        !gpu->copy_h2d((char *)t.to.ptr + o, slot, n))
                        fail(g);
                    else
                        add_progress(g, n, n);
                    free_slots.push_back(slot);
                }
            } else if (kf == K_FILE && kt == K_FILE) {
                if (!file_provider::copy_range(t.from.fd, t.from.off, t.to.fd, t.to.off, t.len))
                    fail(g);
                else
                    add_progress(g, t.len, t.len);
            } else if (kf == K_HOST && kt == K_HOST) {
                memcpy(t.to.ptr, t.from.ptr, t.len);
                add_progress(g, t.len, t.len);
            } else {
                ERROR("unsupported transfer between endpoint kinds " << kf << " and " << kt);
                fail(g);
            }
        }
        if (!g->failed && !dev_out.empty())
            run_device_out(g, dev_out);
        finish(g);
    }

    void finish(const std::shared_ptr<xfer_group_impl_t> &g) {
        std::function<void()> cb;
        {
            std::unique_lock<std::mutex> lk(mtx);
            g->src_done = g->src_total; // ensure waiters wake even after an early failure
            g->all_done = g->all_total;
            g->work_done = true;
            for (int fd : g->fds)
                if (fd >= 0)
                    ::close(fd);
            g->fds.clear();
            if (g->completion && !g->completion_ran) {
                cb = g->completion;
                g->completion_ran = true;
            }
        }
        if (cb)
            cb();
        std::unique_lock<std::mutex> lk(mtx);
        app_cv.notify_all();
    }
};

// ---- xfer_group_t ----------------------------------------------------------

void xfer_group_t::submit(endpoint_t from, endpoint_t to, size_t len) {
    if (!g || len == 0)
        return;
    g->transfers.push_back({from, to, len});
    g->src_total += len;
    g->all_total += len;
}

void *xfer_group_t::alloc(size_t size) {
    if (!g)
        return nullptr;
    g->buffers.emplace_back(new char[size]);
    return g->buffers.back().get();
}

void xfer_group_t::adopt_fd(int fd) {
    if (g)
        g->fds.push_back(fd);
}

bool xfer_group_t::wait_sources() {
    if (!g)
        return true;
    auto *e = g->eng;
    std::unique_lock<std::mutex> lk(e->mtx);
    e->ensure_queued_locked(g);
    e->app_cv.wait(lk, [&] { return g->src_done >= g->src_total; });
    return !g->failed;
}

bool xfer_group_t::wait_completion() {
    if (!g)
        return true;
    auto *e = g->eng;
    std::function<void()> cb;
    {
        std::unique_lock<std::mutex> lk(e->mtx);
        e->ensure_queued_locked(g);
        e->app_cv.wait(lk, [&] { return g->work_done; });
        if (g->completion && !g->completion_ran) {
            cb = g->completion;
            g->completion_ran = true;
        }
    }
    if (cb)
        cb();
    return !g->failed;
}

void xfer_group_t::on_completion(std::function<void()> cont) {
    if (!g)
        return;
    auto *e = g->eng;
    std::function<void()> run_now;
    {
        std::unique_lock<std::mutex> lk(e->mtx);
        if (g->work_done && !g->completion_ran) {
            g->completion_ran = true;
            run_now = std::move(cont);
        } else
            g->completion = std::move(cont);
    }
    if (run_now)
        run_now();
}

// ---- transfer_engine_t -----------------------------------------------------

transfer_engine_t::transfer_engine_t(const config_t &) : pimpl(new impl_t()) { }
transfer_engine_t::~transfer_engine_t() = default;

transfer_engine_t &transfer_engine_t::instance(const config_t &cfg) {
    static transfer_engine_t engine(cfg);
    return engine;
}

xfer_group_t transfer_engine_t::group() {
    auto g = std::make_shared<xfer_group_impl_t>();
    g->eng = pimpl.get();
    return xfer_group_t(g);
}
