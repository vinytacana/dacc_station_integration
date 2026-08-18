#include <cstddef>
#include <cerrno>
#include <signal.h>
#include <time.h>
#include <unistd.h>

namespace {

bool endsWith(const char* value, const char* suffix, std::size_t suffix_size) {
    if (value == nullptr) {
        return false;
    }

    std::size_t value_size = 0;
    while (value[value_size] != '\0') {
        ++value_size;
    }
    if (value_size < suffix_size) {
        return false;
    }

    for (std::size_t i = 0; i < suffix_size; ++i) {
        if (value[value_size - suffix_size + i] != suffix[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

extern "C" int __real_execvp(const char* file, char* const argv[]);

extern "C" int __wrap_execvp(const char* file, char* const argv[]) {
    constexpr char stop_marker[] = "stop-before-exec.sh";
    constexpr char orphan_marker[] = "orphan-marker.sh";
    constexpr char slow_exec_marker[] = "slow-exec.sh";
    bool stop_before_exec = false;
    bool ignore_sigterm = false;
    bool slow_exec = false;

    for (std::size_t i = 0; argv != nullptr && argv[i] != nullptr; ++i) {
        stop_before_exec = stop_before_exec ||
            endsWith(argv[i], stop_marker, sizeof(stop_marker) - 1);
        ignore_sigterm = ignore_sigterm ||
            endsWith(argv[i], orphan_marker, sizeof(orphan_marker) - 1);
        slow_exec = slow_exec ||
            endsWith(argv[i], slow_exec_marker, sizeof(slow_exec_marker) - 1);
    }

    if (ignore_sigterm) {
        struct sigaction ignored_action{};
        ignored_action.sa_handler = SIG_IGN;
        sigemptyset(&ignored_action.sa_mask);
        sigaction(SIGTERM, &ignored_action, nullptr);
    }
    if (stop_before_exec) {
        kill(getpid(), SIGSTOP);
    }
    if (slow_exec) {
        timespec delay{0, 200 * 1000 * 1000};
        while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {
        }
    }
    return __real_execvp(file, argv);
}
