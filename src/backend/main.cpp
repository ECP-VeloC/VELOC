#include "work_queue.hpp"

#include <unistd.h>
#include <signal.h>
#include <sys/file.h>

#define __DEBUG
#include "common/debug.hpp"

static const std::string ready_file = "/dev/shm/veloc-backend-ready-" + std::to_string(getuid());
bool ec_active = true;

void user_handler(int signum) {
    _exit(0);
}

void exit_handler(int signum) {
    if (ec_active)
        MPI_Finalize();
    remove(ready_file.c_str());
    backend_cleanup();
    _exit(signum);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
	std::cout << "Usage: " << argv[0] << " <veloc_config> [--disable-ec]" << std::endl;
	return -1;
    }

    // check if there is another instance running
    int ready_fd = open(ready_file.c_str(), O_RDWR | O_CREAT, 0644);
    if (ready_fd == -1)
        FATAL("cannot open " << ready_file << ", error = " << strerror(errno));
    int locked = flock(ready_fd, LOCK_EX);
    if (locked == -1)
        FATAL("cannot lock " << ready_file << ", error = " << strerror(errno));
    size_t fsize = lseek(ready_fd, 0, SEEK_END);
    pid_t parent_id;
    if (fsize > 0) {
        if (pread(ready_fd, &parent_id, sizeof(pid_t), 0) != sizeof(pid_t))
            FATAL("cannot read PID from " << ready_file << ", please delete it and try again");
        if (kill(parent_id, 0) == 0) {
            INFO("backend already running, PID=" << parent_id);
            return 0;
        }
    }

    // first instance, continue initialization
    parent_id = getpid();
    config_t cfg(argv[1], true);
    if (cfg.is_sync())
	FATAL("configuration requests sync mode, backend is not needed");

    // disable EC on request
    if (argc == 3 && std::string(argv[2]) == "--disable-ec") {
	INFO("EC module disabled by commmand line switch");
	ec_active = false;
    }

    // initialize MPI or fork into deamon mode if EC disabled
    if (ec_active) {
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	DBG("Active backend rank = " << rank);
    } else {
        pid_t child_id = fork();
        if (child_id < 0 || (child_id == 0 && setsid() == -1))
            FATAL("cannot fork to enter daemon mode, error = " << strerror(errno));
        if (child_id > 0) { // parent waits for continue signal
            struct sigaction action;
            action.sa_handler = user_handler;
            sigemptyset(&action.sa_mask);
            sigaction(SIGUSR1, &action, NULL);
            pause();
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    start_main_loop(cfg, MPI_COMM_WORLD, ec_active);

    // init complete, set exit handler and signal parent to continue
    struct sigaction action;
    action.sa_handler = exit_handler;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    if (!ec_active)
        kill(parent_id, SIGUSR1);
    parent_id = getpid();
    if (pwrite(ready_fd, &parent_id, sizeof(pid_t), 0) != sizeof(pid_t))
        FATAL("cannot write PID to " << ready_file << ", error: " << strerror(errno));
    flock(ready_fd, LOCK_UN);
    close(ready_fd);
    pause();

    return 0;
}
