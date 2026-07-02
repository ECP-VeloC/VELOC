#include "common/gpu/gpu_provider.hpp"

#include <cuda_runtime.h>

//#define __DEBUG
#include "common/debug.hpp"

// CUDA implementation of the GPU provider. This translation unit is the only
// place in the whole project that includes CUDA headers; it is compiled only
// when WITH_CUDA is defined (see CMake).
namespace {

class cuda_provider_t : public gpu_provider_t {
public:
    bool is_device(const void *ptr) override {
        if (ptr == nullptr)
            return false;
        cudaPointerAttributes attr;
        cudaError_t err = cudaPointerGetAttributes(&attr, ptr);
        if (err != cudaSuccess) {
            // Unregistered host allocations report an error on older runtimes;
            // clear it and treat the pointer as host memory.
            cudaGetLastError();
            return false;
        }
        // Managed (unified) memory is host-accessible, so we treat only plain
        // device memory as requiring a device-to-host staging copy.
        return attr.type == cudaMemoryTypeDevice;
    }

    void *alloc_pinned(size_t size) override {
        void *ptr = nullptr;
        if (cudaHostAlloc(&ptr, size, cudaHostAllocDefault) != cudaSuccess) {
            cudaGetLastError();
            return nullptr;
        }
        return ptr;
    }

    void free_pinned(void *ptr) override {
        if (ptr != nullptr)
            cudaFreeHost(ptr);
    }

    bool copy_d2h(void *host_dst, const void *dev_src, size_t size) override {
        return cudaMemcpy(host_dst, dev_src, size, cudaMemcpyDeviceToHost) == cudaSuccess;
    }

    bool copy_h2d(void *dev_dst, const void *host_src, size_t size) override {
        return cudaMemcpy(dev_dst, host_src, size, cudaMemcpyHostToDevice) == cudaSuccess;
    }
};

} // anonymous namespace

gpu_provider_t *create_gpu_provider() {
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess || count == 0) {
        cudaGetLastError();
        INFO("no CUDA device available, GPU staging disabled");
        return nullptr;
    }
    INFO("CUDA GPU provider active (" << count << " device(s))");
    return new cuda_provider_t();
}
