#include "common/gpu/gpu_provider.hpp"

#include <cuda_runtime.h>

#include <mutex>
#include <vector>

//#define __DEBUG
#include "common/debug.hpp"

// CUDA implementation of the GPU provider. This translation unit is the only
// place in the whole project that includes CUDA headers; it is compiled only
// when WITH_CUDA is defined (see CMake).
namespace {

class cuda_provider_t : public gpu_provider_t {
    std::vector<void *> device_slots_;
    std::vector<void *> device_free_;
    std::mutex device_mtx_;
    int gpu_id_ = 0;
    int device_count_ = 0;

public:
    cuda_provider_t(int device_count_): device_count_(device_count_), gpu_provider_t() {

    }

    ~cuda_provider_t() override {
        std::lock_guard<std::mutex> lk(device_mtx_);
        for (void *p : device_slots_)
            if (p != nullptr)
                cudaFree(p);
        device_slots_.clear();
        device_free_.clear();
    }

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

    bool init_device_buffer(size_t slot_size, int slots, int rank) override {
        // GPU is assigned in round-robin manner
        // expected is that one rank use one GPU
        gpu_id_ = rank % device_count_;

        if (slots < 1 || slot_size == 0) {
            DBG(
                "GPU buffer slot size and slot count must be > 1, got: slot count: " << 
                slots << ", slot size: " << slot_size << "B\n"
            )
            return false;
        }

        std::lock_guard<std::mutex> lk(device_mtx_);
        if (cudaSetDevice(gpu_id_) != cudaSuccess)
            return false;
        for (int i = 0; i < slots; i++) {
            void *p = nullptr;
            if (cudaMalloc(&p, slot_size) != cudaSuccess) {
                cudaGetLastError();
                for (void *q : device_slots_)
                    if (q != nullptr)
                        cudaFree(q);
                device_slots_.clear();
                device_free_.clear();
                return false;
            }
            device_slots_.push_back(p);
            device_free_.push_back(p);
        }
        return true;
    }

    void *acquire_device_slot() override {
        std::lock_guard<std::mutex> lk(device_mtx_);
        if (device_free_.empty())
            return nullptr;
        void *p = device_free_.back();
        device_free_.pop_back();
        return p;
    }

    void release_device_slot(void *ptr) override {
        if (ptr == nullptr)
            return;
        std::lock_guard<std::mutex> lk(device_mtx_);
        device_free_.push_back(ptr);
    }

    size_t free_device_slots() override {
        std::lock_guard<std::mutex> lk(device_mtx_);
        return device_free_.size();
    }

    bool copy_d2d(void *dst, const void *src, size_t size) override {
        return cudaMemcpy(dst, src, size, cudaMemcpyDeviceToDevice) == cudaSuccess;
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
    return new cuda_provider_t(count);
}
