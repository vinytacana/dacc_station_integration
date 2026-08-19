#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

namespace ipc {

inline constexpr std::size_t kMaxFrameBytes = 64 * 1024;
using MonotonicTimePoint = std::chrono::steady_clock::time_point;
using MonotonicNow = MonotonicTimePoint (*)() noexcept;

enum class SendStatus {
    Ok,
    Disconnected,
    Timeout,
    Desynced,
    TooLarge,
    Error
};

struct SendResult {
    SendStatus status{SendStatus::Error};
    std::size_t bytes_sent{0};
    int err{0};
};

// Desynced means that part of a frame reached the peer before the send failed.
// The caller must close the socket; sending another frame would corrupt framing.

SendResult sendMessage(
    int fd,
    std::string_view payload,
    std::chrono::milliseconds timeout
);

SendResult sendMessage(
    int fd,
    std::string_view payload,
    std::chrono::milliseconds timeout,
    MonotonicNow now
);

class FrameReader {
public:
    void feed(const char* data, std::size_t n);
    std::optional<std::string> next();

    bool overflowed() const noexcept { return overflowed_; }
    std::size_t bufferedBytes() const noexcept { return current_frame_.size(); }
    void reset();

private:
    std::string current_frame_;
    std::deque<std::string> ready_frames_;
    bool overflowed_{false};
};

} // namespace ipc
