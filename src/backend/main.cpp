#include "common/config.hpp"
#include "common/command.hpp"
#include "modules/module_manager.hpp"
#include "common/ipc_queue.hpp"
using namespace ipc_queue;

#include <queue>
#include <future>
#include <sched.h>
#include <unistd.h>

#define __DEBUG
#include "common/debug.hpp"

const unsigned int DEFAULT_PARALLELISM = 64;

int main(int argc, char *argv[]) {
    bool ec_active = true;

    if (argc < 2 || argc > 3) {
	std::cout << "Usage: " << argv[0] << " <veloc_config> [--disable-ec]" << std::endl;
	return 1;
    }

    config_t cfg(argv[1]);
    if (cfg.is_sync()) {
	ERROR("configuration requests sync mode, backend is not needed");
	return 3;
    }
    if (argc == 3 && std::string(argv[2]) == "--disable-ec") {
	INFO("EC module disabled by commmand line switch");
	ec_active = false;
    }

    if (ec_active) {
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	DBG("Active backend rank = " << rank);
    }

    int max_parallelism;
    if (!cfg.get_optional("max_parallelism", max_parallelism))
        max_parallelism = DEFAULT_PARALLELISM;
    INFO("Max number of client requests processed in parallel: " << max_parallelism);
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    for (int i = 0; i < nproc; i++)
        CPU_SET(i, &cpu_mask);

    backend_cleanup();
    backend_t<command_t> command_queue;
    module_manager_t modules;
    modules.add_default_modules(cfg, MPI_COMM_WORLD, ec_active);

    std::queue<std::future<void> > work_queue;
    command_t c;
    while (true) {
	auto f = command_queue.dequeue_any(c);
	work_queue.push(std::async(std::launch::async, [=, &modules] {
                    // lower worker thread priority and set its affinity to whole CPUSET
                    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_mask);
                    nice(10);
		    f(modules.notify_command(c));
		}));
	if (work_queue.size() > (unsigned int)max_parallelism) {
	    work_queue.front().wait();
	    work_queue.pop();
	}
    }

    if (ec_active) {
	MPI_Finalize();
    }

    return 0;
}
