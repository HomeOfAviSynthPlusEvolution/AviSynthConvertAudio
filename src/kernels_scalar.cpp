// Avisynth+
// https://avs-plus.net
//
// This file is part of Avisynth+ which is released under GPL2+ with exception.

// Convert Audio helper functions (Pure C)
// Copyright (c) 2020 Xinyue Lu, (c) 2021 pinterf

#include "kernels.h"
#include <cstdint>
#include <cmath>

namespace audio_convert {

// Ordinary C reference kernels. SIMD routing is implemented separately.

/*
 8 bit: unsigned (middle point 128)
 16,24,32 bit: signed
 32 bit float: -1.0 .. 1.0

 Assymetric range considerations.

 Android: It is implementation dependent whether the positive maximum of 1.0 is included in the interval
 when converting to integer representation

 Method 1 (e.g. Android): smallest number is full scale, 1.0 is clamped to 1.0 minus one LSB
   -0x8000 - 0x7FFF is valid, nominally +1.0 (top of range) is not part of the range [-1.0..1.0)
 Method 2: largest number is full scale
   -0x7FFF - 0x7FFF, while -8000 exceeds lower limit. [-1.0..1.0] + smallest value is theoretically invalid

*/

// until 3.6.1: S16 = (S32 + 0x8000) >> 16   (plain round-before shift)
// Actual: S16 = S32 >> 16
void convert32To16(const void *inbuf, void *outbuf, int count) {
  auto in16 = reinterpret_cast<const int16_t *>(inbuf);
  auto out = reinterpret_cast<int16_t *>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in16[i * 2 + 1];
}

// until 3.6.1: S32 = (S16 << 16) + (unsigned short)(S16 + 32768)
//              0x7fff -> 0x7fffffff, 0x8000 -> 0x80000000
// Actual: S32 = S16 << 16
void convert16To32(const void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<const int16_t *>(inbuf);
  auto out16 = reinterpret_cast<int16_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out16[i * 2] = 0;
    out16[i * 2 + 1] = in[i];
  }
}

void convert32To8(const void* inbuf, void* outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t*>(inbuf);
  auto out = reinterpret_cast<uint8_t*>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in8[i * 4 + 3] + 128;
}

void convert8To32(const void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<const uint8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 4] = 0;
    out8[i * 4 + 1] = 0;
    out8[i * 4 + 2] = 0;
    out8[i * 4 + 3] = in[i] - 128;
  }
}

void convert16To8(const void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t *>(inbuf);
  auto out = reinterpret_cast<uint8_t *>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in8[i * 2 + 1] + 128;
}

// until 3.6.1: S16 = (S8 << 8) + (unsigned short)(S8 + 128)
//              This make 0x7f(255-128) -> 0x7fff & 0x80(0-128) -> 0x8000
// Actual: S16 = (U8-128) << 8
void convert8To16(const void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<const uint8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 2] = 0;
    out8[i * 2 + 1] = in[i] - 128;
  }
}

void convert32To24(const void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 3 + 0] = in8[i * 4 + 1];
    out8[i * 3 + 1] = in8[i * 4 + 2];
    out8[i * 3 + 2] = in8[i * 4 + 3];
  }
}

void convert24To32(const void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 4] = 0;
    out8[i * 4 + 1] = in8[i * 3 + 0];
    out8[i * 4 + 2] = in8[i * 3 + 1];
    out8[i * 4 + 3] = in8[i * 3 + 2];
  }
}

void convert24To16(const void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 2 + 0] = in8[i * 3 + 1];
    out8[i * 2 + 1] = in8[i * 3 + 2];
  }
}

void convert16To24(const void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 3] = 0;
    out8[i * 3 + 1] = in8[i * 2 + 0];
    out8[i * 3 + 2] = in8[i * 2 + 1];
  }
}

void convert24To8(const void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<const int8_t *>(inbuf);
  auto out = reinterpret_cast<uint8_t *>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in8[i * 3 + 2] + 128;
}

void convert8To24(const void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<const uint8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 3] = 0;
    out8[i * 3 + 1] = 0;
    out8[i * 3 + 2] = in[i] - 128;
  }
}

void convert8ToFLT(const void* inbuf, void* outbuf, int count) {
  auto in = reinterpret_cast<const uint8_t*>(inbuf);
  auto out = reinterpret_cast<float*>(outbuf);
  constexpr float divisor = 1.0f / 128.f; // 1 << 7

  for (int i = 0; i < count; i++)
    out[i] = (in[i] - 128) * divisor;
}

void convertFLTTo8(const void* inbuf, void* outbuf, int count) {
  auto in = reinterpret_cast<const float*>(inbuf);
  auto out = reinterpret_cast<uint8_t*>(outbuf);
  constexpr float multiplier = 128.f;
  constexpr float max8 = 127.f;
  constexpr float min8 = -128.f;

  for (int i = 0; i < count; i++) {
    float val = in[i] * multiplier;
    uint8_t result;
    if (std::isnan(val)) result = 128; // NaN becomes silence.
    else if (val >= max8) result = 255;
    else if (val <= min8) result = 0;
    else result = static_cast<int8_t>(val) + 128;
    out[i] = result;
  }
}

void convert16ToFLT(const void* inbuf, void* outbuf, int count) {
  auto in = reinterpret_cast<const int16_t*>(inbuf);
  auto out = reinterpret_cast<float*>(outbuf);
  constexpr float divisor = 1.0f / 32768.f; // 1 << 15

  for (int i = 0; i < count; i++)
    out[i] = in[i] * divisor;
}

void convertFLTTo16(const void* inbuf, void* outbuf, int count) {
  auto in = reinterpret_cast<const float*>(inbuf);
  auto out = reinterpret_cast<int16_t*>(outbuf);
  constexpr float multiplier = 32768.f;
  constexpr float max16 = 32767.f;
  constexpr float min16 = -32768.f;

  for (int i = 0; i < count; i++) {
    float val = in[i] * multiplier;
    int16_t result;
    if (std::isnan(val)) result = 0;
    else if (val >= max16) result = 32767;
    else if (val <= min16) result = (int16_t)-32768;
    else result = static_cast<int16_t>(val);
    out[i] = result;
  }
}

// note for 32 bit conversions: 32 bit integer cannot be represented in float
// 2147483647.0f is 2147483648.0f in reality

void convert32ToFLT(const void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<const int32_t *>(inbuf);
  auto out = reinterpret_cast<float *>(outbuf);
  constexpr float divisor = 1.0f/2147483648.0f;

  for (int i = 0; i < count; i++)
    out[i] = in[i] * divisor;
}

void convertFLTTo32(const void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<const float *>(inbuf);
  auto out = reinterpret_cast<int32_t *>(outbuf);
  constexpr float multiplier = 2147483648.0f;
  constexpr float max32 = 2147483647.0f;
  constexpr float min32 = -2147483648.0f;

  for (int i = 0; i < count; i++) {
    float val = in[i] * multiplier;
    int32_t result;
    if (std::isnan(val)) result = 0;
    else if (val >= max32) result = 0x7FFFFFFF; // 2147483647
    else if (val <= min32) result = 0x80000000; // -2147483648
    else result = static_cast<int32_t>(val);
    out[i] = result;
  }
}

} // namespace audio_convert
