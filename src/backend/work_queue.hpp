#ifndef __WORK_QUEUE
#define __WORK_QUEUE

#include "common/comm_queue.hpp"
#include "common/config.hpp"

#include <mpi.h>

void start_main_loop(const config_t &cfg, MPI_Comm comm, bool ec_active);

#endif
