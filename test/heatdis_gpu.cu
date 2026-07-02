#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <cuda_runtime.h>

#include "include/veloc.h"

/*
    Minimal GPU checkpoint/restart test for VELOC. Protects a mix of host memory
    (an iteration counter) and GPU device memory (a data array), checkpoints, and
    on a subsequent run restarts and verifies the device contents were recovered
    correctly through the transfer engine's device<->host staging path.

    Uses the single-process API (no MPI) so it can run standalone. Run it twice
    with the same config: the first run checkpoints, the second restarts+verifies.
*/

#define N (1u << 20) // number of doubles (~8 MiB)

#define CUDA_CHECK(call)                                                        \
    do {                                                                       \
        cudaError_t _e = (call);                                              \
        if (_e != cudaSuccess) {                                              \
            printf("CUDA error at %s:%d: %s\n", __FILE__, __LINE__,           \
                   cudaGetErrorString(_e));                                   \
            exit(3);                                                          \
        }                                                                    \
    } while (0)

static double expected(unsigned int i, int iter) {
    return (double)((i + (unsigned int)iter) % 7);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <cfg_file>\n", argv[0]);
        return 1;
    }
    if (VELOC_Init_single(0, argv[1]) != VELOC_SUCCESS) {
        printf("Error initializing VELOC! Aborting...\n");
        return 2;
    }

    const unsigned int n = N;
    int iter = 0;
    double *d_data = NULL;
    double *h = (double *)malloc(n * sizeof(double));
    CUDA_CHECK(cudaMalloc(&d_data, n * sizeof(double)));

    // Mixed protection: host scalar (id 0) and device array (id 1).
    VELOC_Mem_protect(0, &iter, 1, sizeof(int));
    VELOC_Mem_protect(1, d_data, n, sizeof(double));

    int v = VELOC_Restart_test("heatdis_gpu", 0);
    if (v > 0) {
        printf("Previous checkpoint found (version %d), restarting...\n", v);
        if (VELOC_Restart("heatdis_gpu", v) != VELOC_SUCCESS) {
            printf("Error restarting! Aborting...\n");
            return 4;
        }
        CUDA_CHECK(cudaMemcpy(h, d_data, n * sizeof(double), cudaMemcpyDeviceToHost));
        int ok = 1;
        for (unsigned int i = 0; i < n; i++) {
            if (h[i] != expected(i, iter)) {
                printf("MISMATCH at %u: got %f expected %f\n", i, h[i], expected(i, iter));
                ok = 0;
                break;
            }
        }
        printf("Restart verification: %s (iter=%d)\n", ok ? "OK" : "FAIL", iter);
        VELOC_Finalize(0);
        cudaFree(d_data);
        free(h);
        return ok ? 0 : 5;
    }

    // Fresh run: fill device memory with a known pattern and checkpoint it.
    iter = 3;
    for (unsigned int i = 0; i < n; i++)
        h[i] = expected(i, iter);
    CUDA_CHECK(cudaMemcpy(d_data, h, n * sizeof(double), cudaMemcpyHostToDevice));
    // Poison the host mirror to prove recovery comes from the checkpoint file.
    memset(h, 0, n * sizeof(double));

    if (VELOC_Checkpoint("heatdis_gpu", iter) != VELOC_SUCCESS) {
        printf("Error checkpointing! Aborting...\n");
        return 6;
    }
    printf("Checkpoint OK (iter=%d)\n", iter);

    VELOC_Finalize(1);
    cudaFree(d_data);
    free(h);
    return 0;
}
