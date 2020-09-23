#include "common/config.hpp"
#include "common/command.hpp"
#include "common/comm_queue.hpp"
#include "modules/module_manager.hpp"

#include <queue>
#include <future>
#include <sched.h>
#include <unistd.h>
#include <signal.h>
#include <sys/file.h>

#define __DEBUG
#include "common/debug.hpp"

static const unsigned int DEFAULT_PARALLELISM = 64;
static const std::string ready_file = "/dev/shm/veloc-backend-ready-" + std::to_string(getuid());
bool ec_active = false;

void exit_handler(int signum) {
    if (ec_active)
        MPI_Finalize();
    remove(ready_file.c_str());
    backend_cleanup();
    exit(signum);
}

void child_handler(int signum) {
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
	std::cout << "Usage: " << argv[0] << " <veloc_config> [--disable-ec]" << std::endl;
	return -1;
    }

    config_t cfg(argv[1]);
    if (cfg.is_sync())
	FATAL("configuration requests sync mode, backend is not needed");

    char host_name[HOST_NAME_MAX] = "";
    gethostname(host_name, HOST_NAME_MAX);
    char *prefix = getenv("VELOC_LOG");
    std::string log_file;
    if (prefix == NULL)
        log_file = "/dev/shm/";
    else
        log_file = std::string(prefix) + "/";
    log_file += "veloc-backend-" + std::string(host_name) + "-" + std::to_string(getuid()) + ".log";

    // grab the ready file lock
    int ready_fd = open(ready_file.c_str(), O_WRONLY | O_CREAT, 0644);
    if (ready_fd == -1)
        FATAL("cannot open " << ready_file << ", error = " << strerror(errno));
    int locked = flock(ready_fd, LOCK_EX);
    if (locked == -1)
        FATAL("cannot lock " << ready_file << ", error = " << strerror(errno));

    // check if an instance is already running
    int log_fd = open(log_file.c_str(), O_WRONLY | O_CREAT, 0644);
    if (log_fd == -1)
        FATAL("cannot open " << log_file << ", error = " << strerror(errno));
    locked = flock(log_fd, LOCK_EX | LOCK_NB);
    if (locked == -1) {
        if (errno == EWOULDBLOCK) {
            INFO("backend already running, only one instance is needed");
            return 0;
        } else
            FATAL("cannot acquire lock on: " << log_file << ", error = " << strerror(errno));
    }
    ftruncate(log_fd, 0);

    // initialization complete, deamonize the backend
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    pid_t parent_id = getpid();
    pid_t child_id = fork();
    if (child_id < 0 || (child_id == 0 && setsid() == -1))
        FATAL("cannot fork to enter daemon mode, error = " << strerror(errno));
    if (child_id > 0) { // parent waits for signal
        close(log_fd);
        action.sa_handler = child_handler;
        sigaction(SIGCHLD, &action, NULL);
        pause();
    }
    close(STDIN_FILENO);
    if (dup2(log_fd, STDOUT_FILENO) < 0 || dup2(log_fd, STDERR_FILENO) < 0)
        FATAL("cannot redirect stdout and stderr to: " << log_file << ", error = " << strerror(errno));

    // disable EC on request
    if (argc == 3 && std::string(argv[2]) == "--disable-ec") {
	INFO("EC module disabled by commmand line switch");
	ec_active = false;
    }

    // initialize MPI
    if (ec_active) {
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	DBG("Active backend rank = " << rank);
    }

    // set up parallelism
    int max_parallelism;
    if (!cfg.get_optional("max_parallelism", max_parallelism))
        max_parallelism = DEFAULT_PARALLELISM;
    INFO("Max number of client requests processed in parallel: " << max_parallelism);
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    for (int i = 0; i < nproc; i++)
        CPU_SET(i, &cpu_mask);

    // initialize command queue
    backend_cleanup();
    backend_t<command_t> command_queue;
    module_manager_t modules;
    modules.add_default_modules(cfg, MPI_COMM_WORLD, ec_active);

    // init complete, signal parent to quit
    action.sa_handler = exit_handler;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    close(ready_fd);
    kill(parent_id, SIGCHLD);

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

    return 0;
}
