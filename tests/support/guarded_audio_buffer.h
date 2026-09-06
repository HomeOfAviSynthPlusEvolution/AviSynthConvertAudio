#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace avsut::test {

inline std::atomic<std::uint32_t> guarded_audio_padding_sequence{0};

inline std::uint8_t next_guarded_audio_padding_sentinel() noexcept {
  const auto sequence = guarded_audio_padding_sequence.fetch_add(1, std::memory_order_relaxed);
  return static_cast<std::uint8_t>((sequence * 97U + 0x3DU) & 0xffU);
}

// Flat storage with independent guards and a byte-level active region.  Audio
// samples are intentionally not represented as an owning typed array because
// packed 24-bit samples have no native C++ element type.
class GuardedAudioBuffer {
 public:
  GuardedAudioBuffer(std::size_t active_bytes, std::size_t padding_bytes = 32,
                     std::size_t alignment = 64, std::size_t alignment_offset = 0,
                     std::uint8_t padding_sentinel = next_guarded_audio_padding_sentinel())
      : active_bytes_(active_bytes),
        padding_bytes_(padding_bytes),
        alignment_(alignment),
        alignment_offset_(alignment_offset),
        padding_sentinel_(padding_sentinel) {
    if (alignment_ == 0 || (alignment_ & (alignment_ - 1)) != 0) {
      throw std::invalid_argument("alignment must be a power of two");
    }
    if (alignment_offset_ >= alignment_) {
      throw std::invalid_argument("alignment offset must be smaller than alignment");
    }
    if (active_bytes_ > std::numeric_limits<std::size_t>::max() - padding_bytes_) {
      throw std::invalid_argument("audio allocation size overflows");
    }
    payload_bytes_ = active_bytes_ + padding_bytes_;
    const auto extra = 2 * kGuardBytes + alignment_ + alignment_offset_;
    if (payload_bytes_ > std::numeric_limits<std::size_t>::max() - extra) {
      throw std::invalid_argument("audio allocation size overflows");
    }

    storage_.resize(payload_bytes_ + extra);
    const auto base = reinterpret_cast<std::uintptr_t>(storage_.data() + kGuardBytes);
    const auto aligned = (base + alignment_ - 1) & ~(alignment_ - 1);
    data_ = reinterpret_cast<std::uint8_t*>(aligned) + alignment_offset_;
    prefix_guard_ = data_ - kGuardBytes;
    suffix_guard_ = data_ + payload_bytes_;
    std::fill(data_, data_ + payload_bytes_, 0);
    reset_sentinels();
  }

  std::uint8_t* data() noexcept { return data_; }
  const std::uint8_t* data() const noexcept { return data_; }
  std::size_t active_bytes() const noexcept { return active_bytes_; }
  std::size_t padding_bytes() const noexcept { return padding_bytes_; }
  std::size_t total_bytes() const noexcept { return payload_bytes_; }

  void reset_sentinels() {
    std::fill(prefix_guard_, prefix_guard_ + kGuardBytes, kGuardSentinel);
    std::fill(suffix_guard_, suffix_guard_ + kGuardBytes, kGuardSentinel);
    std::fill(data_ + active_bytes_, data_ + payload_bytes_, padding_sentinel_);
  }

  void fill_active(std::uint8_t value) { std::fill(data_, data_ + active_bytes_, value); }

  bool guards_intact() const {
    return std::all_of(prefix_guard_, prefix_guard_ + kGuardBytes,
                       [](std::uint8_t value) { return value == kGuardSentinel; }) &&
           std::all_of(suffix_guard_, suffix_guard_ + kGuardBytes,
                       [](std::uint8_t value) { return value == kGuardSentinel; });
  }

  bool padding_intact() const {
    return std::all_of(data_ + active_bytes_, data_ + payload_bytes_,
                       [this](std::uint8_t value) { return value == padding_sentinel_; });
  }

  bool padding_intact_from(std::size_t byte_offset) const {
    if (byte_offset < active_bytes_ || byte_offset > payload_bytes_) {
      throw std::invalid_argument("padding range is outside audio payload");
    }
    return std::all_of(data_ + byte_offset, data_ + payload_bytes_,
                       [this](std::uint8_t value) { return value == padding_sentinel_; });
  }

  bool memory_intact() const { return guards_intact() && padding_intact(); }

  std::vector<std::uint8_t> snapshot_active() const {
    return std::vector<std::uint8_t>(data_, data_ + active_bytes_);
  }

  bool active_matches(const std::vector<std::uint8_t>& snapshot) const {
    return snapshot_active() == snapshot;
  }

  void corrupt_suffix_guard_for_test() { suffix_guard_[0] ^= 1; }

 private:
  static constexpr std::size_t kGuardBytes = 64;
  static constexpr std::uint8_t kGuardSentinel = 0xA5;

  std::size_t active_bytes_;
  std::size_t padding_bytes_;
  std::size_t alignment_;
  std::size_t alignment_offset_;
  std::uint8_t padding_sentinel_;
  std::size_t payload_bytes_{};
  std::vector<std::uint8_t> storage_;
  std::uint8_t* data_{};
  std::uint8_t* prefix_guard_{};
  std::uint8_t* suffix_guard_{};
};

}  // namespace avsut::test
