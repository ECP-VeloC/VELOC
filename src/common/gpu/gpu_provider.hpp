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

    virtual ~gpu_provider_t() { }
};

// Returns a provider instance, or nullptr if GPU support is unavailable.
gpu_provider_t *create_gpu_provider();

#endif // __GPU_PROVIDER_HPP
