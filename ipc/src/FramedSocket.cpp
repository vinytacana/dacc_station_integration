#include "ipc/FramedSocket.hpp"

#include <cerrno>
#include <climits>
#include <poll.h>
#include <sys/socket.h>

namespace ipc {
namespace {

bool isDisconnectedError(int error_code) {
    return error_code == EPIPE || error_code == ECONNRESET || error_code == ENOTCONN;
}

SendResult failedSend(SendStatus status, std::size_t bytes_sent, int error_code) {
    if (bytes_sent > 0 && (status == SendStatus::Timeout || status == SendStatus::Error)) {
        status = SendStatus::Desynced;
    }
    return {status, bytes_sent, error_code};
}

int remainingPollTimeout(std::chrono::steady_clock::time_point deadline) {
    const auto now = std::chrono::steady_clock::now();
    if (now >= deadline) {
        return 0;
    }

    const auto remaining = deadline - now;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
    if (milliseconds < remaining) {
        ++milliseconds;
    }
    if (milliseconds.count() > INT_MAX) {
        return INT_MAX;
    }
    return static_cast<int>(milliseconds.count());
}

} // namespace

SendResult sendMessage(
    int fd,
    std::string_view payload,
    std::chrono::milliseconds timeout
) {
    if (fd < 0) {
        return {SendStatus::Error, 0, EBADF};
    }

    const bool has_delimiter = !payload.empty() && payload.back() == '\n';
    const std::size_t frame_bytes = has_delimiter ? payload.size() - 1 : payload.size();
    if (frame_bytes > kMaxFrameBytes) {
        return {SendStatus::TooLarge, 0, EMSGSIZE};
    }

    std::string framed_payload(payload);
    if (!has_delimiter) {
        framed_payload.push_back('\n');
    }

    if (timeout < std::chrono::milliseconds::zero()) {
        timeout = std::chrono::milliseconds::zero();
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::size_t sent = 0;

    while (sent < framed_payload.size()) {
        const ssize_t count = ::send(
            fd,
            framed_payload.data() + sent,
            framed_payload.size() - sent,
            MSG_DONTWAIT | MSG_NOSIGNAL
        );

        if (count > 0) {
            sent += static_cast<std::size_t>(count);
            continue;
        }
        if (count == 0) {
            return failedSend(SendStatus::Disconnected, sent, 0);
        }

        const int send_error = errno;
        if (send_error == EINTR) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return failedSend(SendStatus::Timeout, sent, EINTR);
            }
            continue;
        }
        if (isDisconnectedError(send_error)) {
            return failedSend(SendStatus::Disconnected, sent, send_error);
        }
        if (send_error != EAGAIN && send_error != EWOULDBLOCK) {
            return failedSend(SendStatus::Error, sent, send_error);
        }

        pollfd descriptor{fd, POLLOUT, 0};
        int poll_result = -1;
        while (poll_result < 0) {
            const int poll_timeout = remainingPollTimeout(deadline);
            if (poll_timeout == 0) {
                return failedSend(SendStatus::Timeout, sent, EAGAIN);
            }
            poll_result = ::poll(&descriptor, 1, poll_timeout);
            if (poll_result < 0 && errno != EINTR) {
                return failedSend(SendStatus::Error, sent, errno);
            }
        }

        if (poll_result == 0) {
            return failedSend(SendStatus::Timeout, sent, EAGAIN);
        }
        if ((descriptor.revents & POLLNVAL) != 0) {
            return failedSend(SendStatus::Error, sent, EBADF);
        }
        if ((descriptor.revents & (POLLERR | POLLHUP)) != 0 &&
            (descriptor.revents & POLLOUT) == 0) {
            return failedSend(SendStatus::Disconnected, sent, EPIPE);
        }
    }

    return {SendStatus::Ok, sent, 0};
}

void FrameReader::feed(const char* data, std::size_t n) {
    if (data == nullptr || n == 0 || overflowed_) {
        return;
    }

    for (std::size_t i = 0; i < n; ++i) {
        const char value = data[i];
        if (value == '\n') {
            if (!current_frame_.empty() && current_frame_.back() == '\r') {
                current_frame_.pop_back();
            }
            ready_frames_.push_back(std::move(current_frame_));
            current_frame_.clear();
            continue;
        }

        if (current_frame_.size() == kMaxFrameBytes) {
            current_frame_.clear();
            overflowed_ = true;
            return;
        }
        current_frame_.push_back(value);
    }
}

std::optional<std::string> FrameReader::next() {
    if (ready_frames_.empty()) {
        return std::nullopt;
    }

    std::string frame = std::move(ready_frames_.front());
    ready_frames_.pop_front();
    return frame;
}

void FrameReader::reset() {
    current_frame_.clear();
    ready_frames_.clear();
    overflowed_ = false;
}

} // namespace ipc
