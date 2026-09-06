#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace avsut::test {

template <typename T>
class PlaneView {
 public:
  using element_type = T;
  using value_type = std::remove_const_t<T>;

  PlaneView(T* data, std::size_t width, std::size_t height, std::size_t pitch_bytes)
      : data_(data), width_(width), height_(height), pitch_bytes_(pitch_bytes) {
    if (width_ > std::numeric_limits<std::size_t>::max() / sizeof(value_type)) {
      throw std::invalid_argument("active row size overflows");
    }
    if ((width_ != 0 && height_ != 0) && data_ == nullptr) {
      throw std::invalid_argument("non-empty plane requires data");
    }
    if (pitch_bytes_ < active_row_bytes()) {
      throw std::invalid_argument("pitch is smaller than active row");
    }
  }

  T* data() const noexcept { return data_; }
  std::size_t width() const noexcept { return width_; }
  std::size_t height() const noexcept { return height_; }
  std::size_t pitch_bytes() const noexcept { return pitch_bytes_; }
  std::size_t active_row_bytes() const noexcept { return width_ * sizeof(value_type); }

  T* row(std::size_t y) const {
    if (y >= height_) {
      throw std::out_of_range("plane row is out of range");
    }
    auto* bytes =
        reinterpret_cast<std::conditional_t<std::is_const_v<T>, const std::uint8_t, std::uint8_t>*>(
            data_);
    return reinterpret_cast<T*>(bytes + y * pitch_bytes_);
  }

  PlaneView<const value_type> as_const() const noexcept {
    return PlaneView<const value_type>(data_, width_, height_, pitch_bytes_);
  }

 private:
  T* data_;
  std::size_t width_;
  std::size_t height_;
  std::size_t pitch_bytes_;
};

}  // namespace avsut::test
