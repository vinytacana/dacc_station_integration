#pragma once

#include <cstddef>
#include <string>

namespace test_support {

void resetSendTestDouble();
void failMatchingSends(
    const std::string& payload_fragment,
    int error_code,
    std::size_t failure_count = 1
);

std::size_t matchingSendFailureCount();
std::string lastFailedSendPayload();

} // namespace test_support
