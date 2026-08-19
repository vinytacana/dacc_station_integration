#pragma once

#include <cstddef>
#include <limits>
#include <optional>
#include <string>

namespace test_support {

using PartialSendHook = void (*)() noexcept;

struct PartialSendPlan {
    std::string payload_fragment;
    std::size_t max_bytes_per_call{0};
    std::size_t successful_partial_sends_before_error{
        std::numeric_limits<std::size_t>::max()
    };
    int error_code{0};
    PartialSendHook after_partial_send{nullptr};
    std::optional<int> socket_fd;
};

void resetSendTestDouble();
void failMatchingSends(
    const std::string& payload_fragment,
    int error_code,
    std::size_t failure_count = 1,
    std::optional<int> socket_fd = std::nullopt
);
void configurePartialSends(const PartialSendPlan& plan);

std::size_t matchingSendFailureCount();
std::size_t successfulPartialSendCount();
std::string lastFailedSendPayload();

} // namespace test_support
