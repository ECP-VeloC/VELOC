#ifndef __GPU_PROVIDER_HPP
#define __GPU_PROVIDER_HPP

#include <cstddef>

// Vendor-neutral abstraction over GPU memory operations. All CUDA (or other
// vendor) headers are confined to the corresponding provider translation unit
// (e.g. cuda_provider.cpp), so the rest of the code never sees them.
//
// The transfer engine uses this to transparently detect device pointers and to
// stage device data through pinned host memory. When no GPU support is compiled
// in (or no device is present), create_gpu_provider() returns nullptr and every
// memory pointer is treated as host memory.
struct gpu_provider_t {
    // Returns true if ptr refers to GPU device memory (as opposed to host memory).
    virtual bool is_device(const void *ptr) = 0;

    // Page-locked (pinned) host memory used as staging buffers. Returns nullptr
    // on failure.
    virtual void *alloc_pinned(size_t size) = 0;
    virtual void free_pinned(void *ptr) = 0;

    // Blocking device<->host copies (executed on the engine progress thread, so
    // they never block the application thread).
    virtual bool copy_d2h(void *host_dst, const void *dev_src, size_t size) = 0;
    virtual bool copy_h2d(void *dev_dst, const void *host_src, size_t size) = 0;

    // Device-side staging pool: `slots` buffers of `slot_size` bytes allocated (per client/rank)
    // once at engine init (single GPU, no cross-GPU D2D for now).
    // rank of issuing process, used to determine device id in multi-GPU environment
    // 
    // expected pattern in mult-GPU env. is rank `N` uses GPU `N`, 
    // and if there are `N` GPUs, there are `N` ranks
    // this is expected to allocate GPU buffer always in device which the MPI rank is using
    virtual bool init_device_buffer(size_t slot_size, int slots, int rank) = 0;
    virtual void *acquire_device_slot() = 0;   // nullptr when all slots busy
    virtual void release_device_slot(void *ptr) = 0;
    virtual size_t free_device_slots() = 0;

    // Blocking device<->device copy within the same GPU.
    virtual bool copy_d2d(void *dst, const void *src, size_t size) = 0;

    virtual ~gpu_provider_t() { }
};

// Returns a provider instance, or nullptr if GPU support is unavailable.
gpu_provider_t *create_gpu_provider();

#endif // __GPU_PROVIDER_HPP
