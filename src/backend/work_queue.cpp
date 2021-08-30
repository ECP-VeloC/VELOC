#include "work_queue.hpp"
#include "common/command.hpp"
#include "modules/module_manager.hpp"

#include <queue>
#include <future>
#include <sched.h>
#include <condition_variable>
#include <mutex>
#include <thread>

void start_main_loop(const config_t &cfg, MPI_Comm comm, bool ec_active) {
    std::condition_variable thread_cond;
    bool init_finished = false;
    std::thread([&]() {
        int max_parallelism;
        if (!cfg.get_optional("max_parallelism", max_parallelism))
            max_parallelism = std::thread::hardware_concurrency();

        cpu_set_t cpu_mask;
        CPU_ZERO(&cpu_mask);
        long nproc = sysconf(_SC_NPROCESSORS_ONLN);
        for (int i = 0; i < nproc; i++)
            CPU_SET(i, &cpu_mask);

        backend_cleanup();
        comm_backend_t<command_t> command_queue;
        module_manager_t modules;
        modules.add_default_modules(cfg, comm, ec_active);
        init_finished = true;
        thread_cond.notify_all();

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
            if (work_queue.size() >= (unsigned int)max_parallelism) {
                work_queue.front().wait();
                work_queue.pop();
            }
        }
    }).detach();
    std::mutex thread_lock;
    std::unique_lock<std::mutex> lock(thread_lock);
    while (!init_finished)
        thread_cond.wait(lock);
}
