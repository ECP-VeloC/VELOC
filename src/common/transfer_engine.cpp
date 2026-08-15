#include "transfer_engine.hpp"
#include "file_provider.hpp"
#include "common/gpu/gpu_provider.hpp"
#include "common/config.hpp"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <algorithm>
#include <utility>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

//#define __DEBUG
#include "common/debug.hpp"

// Compile-time tunables (overridable via -D or the gpu_stage_slots config key).
#ifndef VELOC_XFER_CHUNK
#define VELOC_XFER_CHUNK (64UL << 20)   // 64 MiB per staged chunk
#endif
#ifndef VELOC_HOST_STAGE_SLOTS
#define VELOC_HOST_STAGE_SLOTS 16        // bounded pinned pool: SLOTS * CHUNK bytes
#endif
#ifndef VELOC_GPU_BUFFER_SLOTS
#define VELOC_GPU_BUFFER_SLOTS 8        // GPU HBM staging slots: SLOTS * CHUNK bytes
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
    size_t pending_writes = 0;                    // in-flight pinned->file writes (writer thread)
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
    std::vector<void *> free_slots;  // available pinned slots (mutex-guarded)

    std::mutex slot_mtx;             // guards free_slots (worker pops, writer pushes)
    std::condition_variable slot_cv;

    struct write_job_t { xfer_group_impl_t *g; int fd; size_t off; void *buf; size_t n; };
    std::mutex wq_mtx;
    std::condition_variable wq_cv;
    std::deque<write_job_t> write_q; // pinned->file writes, consumed by the writer thread

    std::mutex mtx;
    std::condition_variable app_cv;  // application waiters (wait_sources/wait_completion)
    std::condition_variable work_cv; // progress thread
    std::deque<std::shared_ptr<xfer_group_impl_t>> ready;
    std::thread worker;
    std::thread writer;
    bool stop = false;


    impl_t(const config_t &cfg, const int rank) {
        int num_host_stage_slots = VELOC_HOST_STAGE_SLOTS;
        cfg.get_optional<int>("host_stage_slots", num_host_stage_slots);
        INFO(
            "Initiating host staging pool: " << num_host_stage_slots << " slots of " << 
            chunk << " bytes on rank " << rank
        );

        gpu = create_gpu_provider();
        if (gpu != nullptr) {
            int num_gpu_buffer_slots = VELOC_GPU_BUFFER_SLOTS;
            cfg.get_optional<int>("gpu_buffer_slots", num_gpu_buffer_slots);
            // expected pattern is that one rank uses one client
            // and one rank occupies one GPU
            INFO(
                "Initiating GPU staging pool: " << num_gpu_buffer_slots << " slots of " << 
                chunk << " bytes on rank " << rank
            );
            if (gpu->init_device_buffer(chunk, num_gpu_buffer_slots, rank)) {
                DBG("GPU device buffer: " << num_gpu_buffer_slots << " slots of " << chunk << " bytes");
            }
            else {
                DBG(
                    "GPU device buffer initiation failed: " << num_gpu_buffer_slots << " slots of " << 
                    chunk << " bytes"
                );
                // assume GPU not available
                gpu = nullptr;
            }
        }

        for (int i = 0; i < num_host_stage_slots; i++) {
            void *s;
            bool pinned;
            if (gpu != nullptr) {
                s = gpu->alloc_pinned(chunk);
                pinned = (s != nullptr);
                if (s == nullptr)
                    s = ::malloc(chunk);
                if (s == nullptr)
                    break;
            }
            else {
                s = ::malloc(chunk);
                pinned = false;
                if (s == nullptr)
                    break;
            }

            slots_all.push_back(s);
            slots_pinned.push_back(pinned);
            free_slots.push_back(s);
        }

        worker = std::thread([this] { run(); });
        writer = std::thread([this] { writer_run(); });
    }

    ~impl_t() {
        {
            std::unique_lock<std::mutex> lk(mtx);
            stop = true;
            work_cv.notify_all();
        }
        {
            std::unique_lock<std::mutex> lk(wq_mtx);
            stop = true;
            wq_cv.notify_all();
        }
        {
            std::unique_lock<std::mutex> lk(slot_mtx);
            slot_cv.notify_all();
        }
        if (worker.joinable())
            worker.join();
        if (writer.joinable())
            writer.join();
        for (size_t i = 0; i < slots_all.size(); i++) {
            if (slots_pinned[i])
                gpu->free_pinned(slots_all[i]);
            else
                ::free(slots_all[i]);
        }
        delete gpu;
    }

    void *take_pinned_slot() {
        std::unique_lock<std::mutex> lk(slot_mtx);
        slot_cv.wait(lk, [&] { return stop || !free_slots.empty(); });
        if (free_slots.empty())
            return nullptr;
        void *s = free_slots.back();
        free_slots.pop_back();
        return s;
    }

    void *try_take_pinned_slot() {
        std::unique_lock<std::mutex> lk(slot_mtx);
        if (free_slots.empty())
            return nullptr;
        void *s = free_slots.back();
        free_slots.pop_back();
        return s;
    }

    void putback_pinned_slot(void *s) {
        std::unique_lock<std::mutex> lk(slot_mtx);
        free_slots.push_back(s);
        slot_cv.notify_one();
    }

    void enqueue_write(const std::shared_ptr<xfer_group_impl_t> &g, void *pinned, const dchunk_t &c) {
        {
            std::unique_lock<std::mutex> lk(mtx);
            g->pending_writes++;
        }
        {
            std::unique_lock<std::mutex> lk(wq_mtx);
            write_q.push_back({g.get(), c.fd, c.foff, pinned, c.n});
            wq_cv.notify_one();
        }
    }

    void writer_run() {
        while (true) {
            write_job_t job;
            {
                std::unique_lock<std::mutex> lk(wq_mtx);
                wq_cv.wait(lk, [&] { return stop || !write_q.empty(); });
                if (write_q.empty()) {
                    if (stop)
                        break;
                    continue;
                }
                job = write_q.front();
                write_q.pop_front();
            }
            bool ok = file_provider::write_range(job.fd, job.off, job.buf, job.n);
            {
                std::unique_lock<std::mutex> lk(mtx);
                job.g->pending_writes--;
                if (ok)
                    job.g->all_done += job.n;
                else
                    job.g->failed = true;
                app_cv.notify_all();
            }
            putback_pinned_slot(job.buf);
        }
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
        // Greedy device->staging drain: copy the application's device memory
        // into a GPU HBM slot first (fast D2D, releases the app memory so
        // wait_sources() returns immediately). Once the GPU buffer is exhausted,
        // drain one staged GPU slot into pinned (D2H) to recycle the slot; when
        // even that is not possible, stage straight into pinned. Every filled
        // pinned slot is enqueued on the writer thread immediately, so pinned
        // slots flow back as the writer drains them and this worker never blocks
        // on file I/O.
        std::deque<std::pair<void *, dchunk_t>> gpu_staged;
        auto now = std::chrono::steady_clock::now;
        for (auto &c : chunks) {
            if (g->failed)
                break;
            // 1) Fast path: capture into a free GPU HBM slot.
            void *gslot = (gpu != nullptr) ? gpu->acquire_device_slot() : nullptr;
            if (gslot != nullptr) {
                auto t0 = now();
                if (gpu->copy_d2d(gslot, c.dev, c.n)) {
                    add_progress(g, c.n, 0); // device source released
                    gpu_staged.emplace_back(gslot, c);
                    continue;
                }
                gpu->release_device_slot(gslot);
            }
            // 2) GPU buffer full: recycle one staged GPU slot through pinned so
            //    this chunk can take the freed GPU slot immediately.
            if (!gpu_staged.empty()) {
                auto gs = gpu_staged.front();
                gpu_staged.pop_front();
                void *slot = take_pinned_slot();
                if (slot == nullptr) {
                    fail(g);
                    break;
                }
                auto t0 = now();
                if (gpu->copy_d2h(slot, gs.first, gs.second.n)) {
                    enqueue_write(g, slot, gs.second);
                } else {
                    fail(g);
                    putback_pinned_slot(slot);
                }
                gpu->release_device_slot(gs.first);
                // Re-acquire the just-freed slot and capture this chunk into it.
                gslot = gpu->acquire_device_slot();
                if (gslot != nullptr) {
                    t0 = now();
                    if (gpu->copy_d2d(gslot, c.dev, c.n)) {
                        add_progress(g, c.n, 0); // device source released
                        gpu_staged.emplace_back(gslot, c);
                        continue;
                    }
                    gpu->release_device_slot(gslot);
                }
                // Fall through to pinned fallback if the D2D capture failed.
            }
            // 3) Fallback: stage straight into pinned (freed by the writer).
            void *slot = take_pinned_slot();
            if (slot == nullptr) {
                fail(g);
                break;
            }
            auto t0 = now();
            if (!gpu->copy_d2h(slot, c.dev, c.n)) {
                fail(g);
                putback_pinned_slot(slot);
                break;
            }
            add_progress(g, c.n, 0); // device source released
            enqueue_write(g, slot, c);
        }
        // Drain any GPU slots still staged (e.g. on early failure or end of input).
        while (!gpu_staged.empty()) {
            if (g->failed)
                break;
            auto gs = gpu_staged.front();
            gpu_staged.pop_front();
            void *slot = take_pinned_slot();
            if (slot == nullptr) {
                fail(g);
                break;
            }
            auto t0 = now();
            if (gpu->copy_d2h(slot, gs.first, gs.second.n)) {
                enqueue_write(g, slot, gs.second);
            } else {
                fail(g);
                putback_pinned_slot(slot);
            }
            gpu->release_device_slot(gs.first);
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
                    void *slot = try_take_pinned_slot();
                    if (slot == nullptr) {
                        ERROR("no staging slot available for device restore");
                        fail(g);
                        break;
                    }
                    auto t0 = std::chrono::steady_clock::now();
                    if (!file_provider::read_range(t.from.fd, t.from.off + o, slot, n) ||
                        !gpu->copy_h2d((char *)t.to.ptr + o, slot, n)) {
                        fail(g);
                        break;
                    }
                    add_progress(g, n, n);
                    putback_pinned_slot(slot);
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
            // Wait until every enqueued pinned->file write is durable; the writer
            // thread drains them and notifies. wait_sources() is unaffected.
            app_cv.wait(lk, [&] { return g->pending_writes == 0; });
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

transfer_engine_t::transfer_engine_t(const config_t &cfg, const int rank) : pimpl(new impl_t(cfg, rank)) { }
transfer_engine_t::~transfer_engine_t() = default;

transfer_engine_t &transfer_engine_t::instance(const config_t &cfg, const int rank) {
    static transfer_engine_t engine(cfg, rank);
    return engine;
}

xfer_group_t transfer_engine_t::group() {
    auto g = std::make_shared<xfer_group_impl_t>();
    g->eng = pimpl.get();
    return xfer_group_t(g);
}
