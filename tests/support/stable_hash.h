#pragma once

#include "support/plane_view.h"

#define XXH_STATIC_LINKING_ONLY
#define XXH_INLINE_ALL
#include <xxhash.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace avsut::test {

template <typename T>
std::uint64_t hash_active(PlaneView<const T> view) {
  XXH3_state_t state;
  if (XXH3_64bits_reset(&state) == XXH_ERROR) {
    throw std::runtime_error("XXH3 reset failed");
  }
  for (std::size_t y = 0; y < view.height(); ++y) {
    if (XXH3_64bits_update(&state, view.row(y), view.active_row_bytes()) == XXH_ERROR) {
      throw std::runtime_error("XXH3 update failed");
    }
  }
  return XXH3_64bits_digest(&state);
}

inline std::string format_hash(std::uint64_t hash) {
  std::ostringstream stream;
  stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << hash;
  return stream.str();
}

}  // namespace avsut::test
