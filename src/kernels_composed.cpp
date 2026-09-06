// C fallback composition developed in AviSynthMinus and AviSynthConvertAudio.
// Based on the AviSynth audio conversion implementation.
// Licensed under GPL version 2 or later with the inherited AviSynth
// linking exception. See LICENSE.
#include "kernels.h"
#include <algorithm>
#include <cstdint>

namespace audio_convert {

// Keep the original two-stage numerical semantics without modifying input,
// allocating heap storage, or making the host arrange the intermediate steps.
void convert24ToFLT(const void* input, void* output, int count) {
  const auto* in = static_cast<const uint8_t*>(input);
  auto* out = static_cast<float*>(output);
  alignas(64) int32_t scratch[256];
  while (count > 0) {
    const int n = std::min(count, 256);
    convert24To32(in, scratch, n);
    convert32ToFLT(scratch, out, n);
    in += n * 3;
    out += n;
    count -= n;
  }
}
void convertFLTTo24(const void* input, void* output, int count) {
  const auto* in = static_cast<const float*>(input);
  auto* out = static_cast<uint8_t*>(output);
  alignas(64) int32_t scratch[256];
  while (count > 0) {
    const int n = std::min(count, 256);
    convertFLTTo32(in, scratch, n);
    convert32To24(scratch, out, n);
    in += n;
    out += n * 3;
    count -= n;
  }
}

} // namespace audio_convert
