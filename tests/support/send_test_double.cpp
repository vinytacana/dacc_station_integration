#include "send_test_double.hpp"

#include <cerrno>
#include <mutex>
#include <string>
#include <string_view>
#include <sys/socket.h>

namespace {

struct SendDoubleState {
    std::mutex mutex;
    std::string payload_fragment;
    std::string last_failed_payload;
    int error_code{0};
    std::size_t remaining_failures{0};
    std::size_t matching_failures{0};
};

SendDoubleState& state() {
    static SendDoubleState instance;
    return instance;
}

} // namespace

namespace test_support {

void resetSendTestDouble() {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    current.payload_fragment.clear();
    current.last_failed_payload.clear();
    current.error_code = 0;
    current.remaining_failures = 0;
    current.matching_failures = 0;
}

void failMatchingSends(
    const std::string& payload_fragment,
    int error_code,
    std::size_t failure_count
) {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    current.payload_fragment = payload_fragment;
    current.last_failed_payload.clear();
    current.error_code = error_code;
    current.remaining_failures = failure_count;
    current.matching_failures = 0;
}

std::size_t matchingSendFailureCount() {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    return current.matching_failures;
}

std::string lastFailedSendPayload() {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    return current.last_failed_payload;
}

} // namespace test_support

extern "C" ssize_t __real_send(int socket, const void* buffer, size_t length, int flags);

extern "C" ssize_t __wrap_send(int socket, const void* buffer, size_t length, int flags) {
    SendDoubleState& current = state();
    {
        std::lock_guard<std::mutex> lock(current.mutex);
        const std::string_view payload(
            static_cast<const char*>(buffer),
            buffer == nullptr ? 0 : length
        );
        if (current.remaining_failures > 0 &&
            !current.payload_fragment.empty() &&
            payload.find(current.payload_fragment) != std::string_view::npos) {
            --current.remaining_failures;
            ++current.matching_failures;
            current.last_failed_payload.assign(payload.data(), payload.size());
            errno = current.error_code;
            return -1;
        }
    }

    return __real_send(socket, buffer, length, flags);
}
