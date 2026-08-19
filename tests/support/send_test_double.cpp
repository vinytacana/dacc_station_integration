#include "send_test_double.hpp"

#include <algorithm>
#include <cerrno>
#include <mutex>
#include <optional>
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
    std::optional<int> failure_socket_fd;

    test_support::PartialSendPlan partial_plan;
    bool partial_plan_active{false};
    int partial_plan_socket{-1};
    std::size_t successful_partial_sends{0};
    std::string partial_frame_payload;
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
    current.failure_socket_fd.reset();
    current.partial_plan = {};
    current.partial_plan_active = false;
    current.partial_plan_socket = -1;
    current.successful_partial_sends = 0;
    current.partial_frame_payload.clear();
}

void failMatchingSends(
    const std::string& payload_fragment,
    int error_code,
    std::size_t failure_count,
    std::optional<int> socket_fd
) {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    current.payload_fragment = payload_fragment;
    current.last_failed_payload.clear();
    current.error_code = error_code;
    current.remaining_failures = failure_count;
    current.matching_failures = 0;
    current.failure_socket_fd = socket_fd;
}

void configurePartialSends(const PartialSendPlan& plan) {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    current.partial_plan = plan;
    current.partial_plan_active = false;
    current.partial_plan_socket = -1;
    current.successful_partial_sends = 0;
    current.partial_frame_payload.clear();
}

std::size_t matchingSendFailureCount() {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    return current.matching_failures;
}

std::size_t successfulPartialSendCount() {
    SendDoubleState& current = state();
    std::lock_guard<std::mutex> lock(current.mutex);
    return current.successful_partial_sends;
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
    std::size_t limited_length = length;
    test_support::PartialSendHook after_partial_send = nullptr;
    bool partial_plan_applies = false;

    {
        std::lock_guard<std::mutex> lock(current.mutex);
        const std::string_view payload(
            static_cast<const char*>(buffer),
            buffer == nullptr ? 0 : length
        );
        if (current.remaining_failures > 0 &&
            (!current.failure_socket_fd || *current.failure_socket_fd == socket) &&
            !current.payload_fragment.empty() &&
            payload.find(current.payload_fragment) != std::string_view::npos) {
            --current.remaining_failures;
            ++current.matching_failures;
            current.last_failed_payload.assign(payload.data(), payload.size());
            errno = current.error_code;
            return -1;
        }

        const bool partial_fd_matches =
            !current.partial_plan.socket_fd || *current.partial_plan.socket_fd == socket;
        if (!current.partial_plan_active &&
            partial_fd_matches &&
            current.partial_plan.max_bytes_per_call > 0 &&
            !current.partial_plan.payload_fragment.empty() &&
            payload.find(current.partial_plan.payload_fragment) != std::string_view::npos) {
            current.partial_plan_active = true;
            current.partial_plan_socket = socket;
            current.partial_frame_payload.assign(payload.data(), payload.size());
        }

        if (current.partial_plan_active && current.partial_plan_socket == socket) {
            if (current.partial_plan.error_code != 0 &&
                current.successful_partial_sends >=
                    current.partial_plan.successful_partial_sends_before_error) {
                ++current.matching_failures;
                current.last_failed_payload = current.partial_frame_payload;
                errno = current.partial_plan.error_code;
                return -1;
            }

            limited_length = std::min(length, current.partial_plan.max_bytes_per_call);
            after_partial_send = current.partial_plan.after_partial_send;
            partial_plan_applies = true;
        }
    }

    const ssize_t result = __real_send(socket, buffer, limited_length, flags);
    if (partial_plan_applies && result > 0 && static_cast<std::size_t>(result) < length) {
        {
            std::lock_guard<std::mutex> lock(current.mutex);
            ++current.successful_partial_sends;
        }
        if (after_partial_send != nullptr) {
            after_partial_send();
        }
    }
    return result;
}
