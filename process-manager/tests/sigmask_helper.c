#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    sigset_t current_mask;
    if (sigprocmask(SIG_SETMASK, NULL, &current_mask) != 0) {
        return 2;
    }

    const int sigchld_blocked = sigismember(&current_mask, SIGCHLD);
    printf("SIGCHLD blocked: %s\n", sigchld_blocked == 1 ? "yes" : "no");
    fflush(stdout);
    if (sigchld_blocked != 0) {
        return 42;
    }

    const pid_t child_pid = fork();
    if (child_pid < 0) {
        return 3;
    }
    if (child_pid == 0) {
        _exit(0);
    }

    int status = 0;
    pid_t wait_result;
    do {
        wait_result = waitpid(child_pid, &status, 0);
    } while (wait_result < 0 && errno == EINTR);
    if (wait_result != child_pid) {
        return 4;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 4;
}
