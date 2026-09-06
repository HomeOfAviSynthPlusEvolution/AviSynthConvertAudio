#pragma once

#include "kernels.h"

#include "support/comparators.h"

#include "support/guarded_audio_buffer.h"
#include "support/stable_hash.h"
#include "support/variant_registry.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace avsut::test {

using namespace audio_convert;

using AudioConvertFunction = void (*)(const void*, void*, int);

enum class AudioFormat { U8, S16, S24, S32, F32 };

inline const char* audio_format_name(AudioFormat format) {
  switch (format) {
    case AudioFormat::U8:
      return "U8";
    case AudioFormat::S16:
      return "S16";
    case AudioFormat::S24:
      return "S24";
    case AudioFormat::S32:
      return "S32";
    case AudioFormat::F32:
      return "F32";
  }
  return "Unknown";
}

inline std::size_t audio_format_bytes(AudioFormat format) {
  switch (format) {
    case AudioFormat::U8:
      return 1;
    case AudioFormat::S16:
      return 2;
    case AudioFormat::S24:
      return 3;
    case AudioFormat::S32:
    case AudioFormat::F32:
      return 4;
  }
  throw std::invalid_argument("unknown audio format");
}

inline std::size_t audio_alignment_offset(AudioFormat format) {
  switch (format) {
    case AudioFormat::U8:
      return 1;
    case AudioFormat::S16:
      return 2;
    case AudioFormat::S24:
      return 3;
    case AudioFormat::S32:
    case AudioFormat::F32:
      return 4;
  }
  return 0;
}

inline std::string audio_variant_name(const Variant<AudioConvertFunction>& variant) {
  std::string result = "Variant";
  bool capitalize = true;
  for (const char character : variant.name) {
    if (character == '_' || character == '-' || character == '.') {
      capitalize = true;
      continue;
    }
    result.push_back(capitalize && character >= 'a' && character <= 'z'
                         ? static_cast<char>(character - ('a' - 'A'))
                         : character);
    capitalize = false;
  }
  return result;
}

struct AudioIntegerCase {
  AudioFormat source_format{};
  AudioFormat destination_format{};
  std::size_t count{};
  std::size_t source_alignment_offset{};
  std::size_t destination_alignment_offset{};
  Variant<AudioConvertFunction> variant;
  std::string expected_hash;
  std::string name;
};

inline std::string audio_integer_case_name(AudioFormat source_format,
                                           AudioFormat destination_format, std::size_t count,
                                           std::size_t source_offset,
                                           std::size_t destination_offset,
                                           const Variant<AudioConvertFunction>& variant) {
  std::ostringstream stream;
  stream << audio_format_name(source_format) << "To" << audio_format_name(destination_format)
         << "_Count" << count << "_SrcOffset" << source_offset << "_DstOffset" << destination_offset
         << "_PatternBoundaryValues_" << audio_variant_name(variant);
  return stream.str();
}

inline AudioIntegerCase make_audio_integer_case(AudioFormat source_format,
                                                AudioFormat destination_format, std::size_t count,
                                                Variant<AudioConvertFunction> variant,
                                                std::string expected_hash = {}) {
  AudioIntegerCase result{source_format,
                          destination_format,
                          count,
                          audio_alignment_offset(source_format),
                          audio_alignment_offset(destination_format),
                          std::move(variant),
                          std::move(expected_hash),
                          {}};
  result.name = audio_integer_case_name(result.source_format, result.destination_format,
                                        result.count, result.source_alignment_offset,
                                        result.destination_alignment_offset, result.variant);
  return result;
}

inline void PrintTo(const AudioIntegerCase& test_case, std::ostream* stream) {
  *stream << test_case.name;
}

inline std::int64_t integer_anchor(AudioFormat format, std::size_t index) {
  switch (format) {
    case AudioFormat::U8: {
      constexpr std::array<std::int64_t, 10> values{0, 1, 2, 127, 128, 129, 254, 255, 42, 213};
      return values[index % values.size()];
    }
    case AudioFormat::S16: {
      constexpr std::array<std::int64_t, 12> values{std::numeric_limits<std::int16_t>::min(),
                                                    -32767,
                                                    -32768,
                                                    -257,
                                                    -256,
                                                    -1,
                                                    0,
                                                    1,
                                                    255,
                                                    256,
                                                    32766,
                                                    std::numeric_limits<std::int16_t>::max()};
      return values[index % values.size()];
    }
    case AudioFormat::S32: {
      constexpr std::array<std::int64_t, 14> values{
          std::numeric_limits<std::int32_t>::min(),
          static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) + 1,
          -2147483647LL,
          -16777217LL,
          -16777216LL,
          -65536LL,
          -1LL,
          0LL,
          1LL,
          65535LL,
          65536LL,
          16777215LL,
          16777216LL,
          std::numeric_limits<std::int32_t>::max()};
      return values[index % values.size()];
    }
    case AudioFormat::S24: {
      constexpr std::array<std::int64_t, 14> values{
          -8388608LL, -8388607LL, -8388606LL, -65536LL, -257LL,  -256LL,    -1LL,
          0LL,        1LL,        255LL,      256LL,    65535LL, 8388606LL, 8388607LL};
      return values[index % values.size()];
    }
    case AudioFormat::F32:
      break;
  }
  throw std::invalid_argument("integer anchor requested for non-integer format");
}

inline void write_u16_le(std::uint8_t* destination, std::uint16_t value) {
  destination[0] = static_cast<std::uint8_t>(value);
  destination[1] = static_cast<std::uint8_t>(value >> 8);
}

inline void write_u32_le(std::uint8_t* destination, std::uint32_t value) {
  destination[0] = static_cast<std::uint8_t>(value);
  destination[1] = static_cast<std::uint8_t>(value >> 8);
  destination[2] = static_cast<std::uint8_t>(value >> 16);
  destination[3] = static_cast<std::uint8_t>(value >> 24);
}

inline void write_u24_le(std::uint8_t* destination, std::uint32_t value) {
  destination[0] = static_cast<std::uint8_t>(value);
  destination[1] = static_cast<std::uint8_t>(value >> 8);
  destination[2] = static_cast<std::uint8_t>(value >> 16);
}

inline void fill_integer_audio_source(GuardedAudioBuffer& buffer, AudioFormat format,
                                      std::size_t count) {
  const auto bytes = audio_format_bytes(format);
  if (buffer.active_bytes() != count * bytes) {
    throw std::invalid_argument("audio source size does not match count");
  }
  for (std::size_t index = 0; index < count; ++index) {
    auto* destination = buffer.data() + index * bytes;
    const auto value = integer_anchor(format, index);
    switch (format) {
      case AudioFormat::U8:
        destination[0] = static_cast<std::uint8_t>(value);
        break;
      case AudioFormat::S16:
        write_u16_le(destination, static_cast<std::uint16_t>(static_cast<std::int16_t>(value)));
        break;
      case AudioFormat::S32:
        write_u32_le(destination, static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
        break;
      case AudioFormat::S24:
        write_u24_le(destination,
                     static_cast<std::uint32_t>(static_cast<std::int32_t>(value)) & 0x00ffffffU);
        break;
      case AudioFormat::F32:
        throw std::invalid_argument("unsupported integer source format");
    }
  }
}

inline void copy_bytes(std::uint8_t* destination, const std::uint8_t* source, std::size_t count) {
  std::memcpy(destination, source, count);
}

inline void convert_integer_reference(AudioFormat source_format, AudioFormat destination_format,
                                      const std::uint8_t* source, std::uint8_t* destination,
                                      std::size_t count) {
  if (source_format == AudioFormat::S32 && destination_format == AudioFormat::S16) {
    for (std::size_t i = 0; i < count; ++i) copy_bytes(destination + i * 2, source + i * 4 + 2, 2);
    return;
  }
  if (source_format == AudioFormat::S16 && destination_format == AudioFormat::S32) {
    for (std::size_t i = 0; i < count; ++i) {
      destination[i * 4] = 0;
      destination[i * 4 + 1] = 0;
      copy_bytes(destination + i * 4 + 2, source + i * 2, 2);
    }
    return;
  }
  if (source_format == AudioFormat::S32 && destination_format == AudioFormat::U8) {
    for (std::size_t i = 0; i < count; ++i)
      destination[i] = static_cast<std::uint8_t>(static_cast<std::int8_t>(source[i * 4 + 3]) + 128);
    return;
  }
  if (source_format == AudioFormat::U8 && destination_format == AudioFormat::S32) {
    for (std::size_t i = 0; i < count; ++i) {
      destination[i * 4] = 0;
      destination[i * 4 + 1] = 0;
      destination[i * 4 + 2] = 0;
      destination[i * 4 + 3] = static_cast<std::uint8_t>(static_cast<std::int16_t>(source[i]) -
                                                         static_cast<std::int16_t>(128));
    }
    return;
  }
  if (source_format == AudioFormat::S16 && destination_format == AudioFormat::U8) {
    for (std::size_t i = 0; i < count; ++i)
      destination[i] = static_cast<std::uint8_t>(static_cast<std::int8_t>(source[i * 2 + 1]) + 128);
    return;
  }
  if (source_format == AudioFormat::U8 && destination_format == AudioFormat::S16) {
    for (std::size_t i = 0; i < count; ++i) {
      destination[i * 2] = 0;
      destination[i * 2 + 1] = static_cast<std::uint8_t>(static_cast<std::int16_t>(source[i]) -
                                                         static_cast<std::int16_t>(128));
    }
    return;
  }
  if (source_format == AudioFormat::S32 && destination_format == AudioFormat::S24) {
    for (std::size_t i = 0; i < count; ++i) copy_bytes(destination + i * 3, source + i * 4 + 1, 3);
    return;
  }
  if (source_format == AudioFormat::S24 && destination_format == AudioFormat::S32) {
    for (std::size_t i = 0; i < count; ++i) {
      destination[i * 4] = 0;
      copy_bytes(destination + i * 4 + 1, source + i * 3, 3);
    }
    return;
  }
  if (source_format == AudioFormat::S24 && destination_format == AudioFormat::S16) {
    for (std::size_t i = 0; i < count; ++i) copy_bytes(destination + i * 2, source + i * 3 + 1, 2);
    return;
  }
  if (source_format == AudioFormat::S16 && destination_format == AudioFormat::S24) {
    for (std::size_t i = 0; i < count; ++i) {
      destination[i * 3] = 0;
      copy_bytes(destination + i * 3 + 1, source + i * 2, 2);
    }
    return;
  }
  if (source_format == AudioFormat::S24 && destination_format == AudioFormat::U8) {
    for (std::size_t i = 0; i < count; ++i)
      destination[i] = static_cast<std::uint8_t>(static_cast<std::int8_t>(source[i * 3 + 2]) + 128);
    return;
  }
  if (source_format == AudioFormat::U8 && destination_format == AudioFormat::S24) {
    for (std::size_t i = 0; i < count; ++i) {
      destination[i * 3] = 0;
      destination[i * 3 + 1] = 0;
      destination[i * 3 + 2] = static_cast<std::uint8_t>(static_cast<std::int16_t>(source[i]) -
                                                         static_cast<std::int16_t>(128));
    }
    return;
  }
  throw std::invalid_argument("unsupported basic integer conversion");
}

inline std::uint64_t hash_audio_active(const GuardedAudioBuffer& buffer) {
  const PlaneView<const std::uint8_t> view(buffer.data(), buffer.active_bytes(), 1,
                                           buffer.active_bytes());
  return hash_active(view);
}

inline ::testing::AssertionResult compare_audio_exact(const GuardedAudioBuffer& expected,
                                                      const GuardedAudioBuffer& actual) {
  if (expected.active_bytes() != actual.active_bytes()) {
    return ::testing::AssertionFailure() << "active byte count mismatch";
  }
  for (std::size_t index = 0; index < expected.active_bytes(); ++index) {
    if (expected.data()[index] != actual.data()[index]) {
      return ::testing::AssertionFailure()
             << "byte=" << index << " expected=" << static_cast<unsigned>(expected.data()[index])
             << " actual=" << static_cast<unsigned>(actual.data()[index]);
    }
  }
  return ::testing::AssertionSuccess();
}

inline void run_audio_integer_case(const AudioIntegerCase& test_case) {
  const auto source_bytes = test_case.count * audio_format_bytes(test_case.source_format);
  const auto destination_bytes = test_case.count * audio_format_bytes(test_case.destination_format);
  GuardedAudioBuffer source(source_bytes, 64, 64, test_case.source_alignment_offset);
  GuardedAudioBuffer expected(destination_bytes, 64, 64, test_case.destination_alignment_offset);
  GuardedAudioBuffer actual(destination_bytes, 64, 64, test_case.destination_alignment_offset);

  fill_integer_audio_source(source, test_case.source_format, test_case.count);
  const auto source_snapshot = source.snapshot_active();
  expected.fill_active(0xCD);
  actual.fill_active(0xCD);
  convert_integer_reference(test_case.source_format, test_case.destination_format, source.data(),
                            expected.data(), test_case.count);

  test_case.variant.function(source.data(), actual.data(), static_cast<int>(test_case.count));

  EXPECT_TRUE(compare_audio_exact(expected, actual))
      << test_case.name << " reference mismatch for variant " << test_case.variant.name;
  if (!test_case.expected_hash.empty()) {
    EXPECT_EQ(format_hash(hash_audio_active(actual)), test_case.expected_hash) << test_case.name;
  }
  EXPECT_TRUE(source.active_matches(source_snapshot)) << test_case.name << " modified source";
  EXPECT_TRUE(source.memory_intact()) << test_case.name << " source guard or padding corruption";
  EXPECT_TRUE(expected.memory_intact())
      << test_case.name << " reference guard or padding corruption";
  EXPECT_TRUE(actual.memory_intact()) << test_case.name << " output guard or padding corruption";
}

struct AudioFloatCase {
  AudioFormat source_format{};
  AudioFormat destination_format{};
  std::size_t count{};
  std::size_t source_alignment_offset{};
  std::size_t destination_alignment_offset{};
  Variant<AudioConvertFunction> variant;
  std::string expected_hash;
  std::string name;
};

inline std::string audio_float_case_name(AudioFormat source_format, AudioFormat destination_format,
                                         std::size_t count, std::size_t source_offset,
                                         std::size_t destination_offset,
                                         const Variant<AudioConvertFunction>& variant) {
  std::ostringstream stream;
  stream << audio_format_name(source_format) << "To" << audio_format_name(destination_format)
         << "_Count" << count << "_SrcOffset" << source_offset << "_DstOffset" << destination_offset
         << "_PatternBoundaryValues_" << audio_variant_name(variant);
  return stream.str();
}

inline AudioFloatCase make_audio_float_case(AudioFormat source_format,
                                            AudioFormat destination_format, std::size_t count,
                                            Variant<AudioConvertFunction> variant,
                                            std::string expected_hash = {}) {
  AudioFloatCase result{source_format,
                        destination_format,
                        count,
                        audio_alignment_offset(source_format),
                        audio_alignment_offset(destination_format),
                        std::move(variant),
                        std::move(expected_hash),
                        {}};
  result.name = audio_float_case_name(result.source_format, result.destination_format, result.count,
                                      result.source_alignment_offset,
                                      result.destination_alignment_offset, result.variant);
  return result;
}

inline void PrintTo(const AudioFloatCase& test_case, std::ostream* stream) {
  *stream << test_case.name;
}

inline float float_anchor(std::size_t index) {
  constexpr std::array<float, 18> values{-1.25F,
                                         -1.0F,
                                         -0.99999F,
                                         -0.5F,
                                         -0.0078125F,
                                         -0.000030517578125F,
                                         -0.0000000004656612873077392578125F,
                                         -0.0F,
                                         0.0F,
                                         0.0000000004656612873077392578125F,
                                         0.000030517578125F,
                                         0.0078125F,
                                         0.5F,
                                         0.99999F,
                                         1.0F,
                                         1.25F,
                                         0.125F,
                                         -0.125F};
  return values[index % values.size()];
}

inline void write_float(std::uint8_t* destination, float value) {
  std::memcpy(destination, &value, sizeof(value));
}

inline float read_float(const std::uint8_t* source) {
  float value{};
  std::memcpy(&value, source, sizeof(value));
  return value;
}

inline std::int16_t read_s16_le(const std::uint8_t* source) {
  const auto bits =
      static_cast<std::uint16_t>(source[0]) | (static_cast<std::uint16_t>(source[1]) << 8);
  return static_cast<std::int16_t>(bits);
}

inline std::int32_t read_s32_le(const std::uint8_t* source) {
  const auto bits =
      static_cast<std::uint32_t>(source[0]) | (static_cast<std::uint32_t>(source[1]) << 8) |
      (static_cast<std::uint32_t>(source[2]) << 16) | (static_cast<std::uint32_t>(source[3]) << 24);
  return static_cast<std::int32_t>(bits);
}

inline float integer_to_float_reference(AudioFormat source_format, const std::uint8_t* source) {
  switch (source_format) {
    case AudioFormat::U8:
      return (static_cast<int>(source[0]) - 128) * (1.0F / 128.0F);
    case AudioFormat::S16:
      return read_s16_le(source) * (1.0F / 32768.0F);
    case AudioFormat::S24: {
      const std::int32_t s32 = static_cast<std::int32_t>(
          (static_cast<std::uint32_t>(source[0]) << 8) |
          (static_cast<std::uint32_t>(source[1]) << 16) |
          (static_cast<std::uint32_t>(source[2]) << 24));
      return s32 * (1.0F / 2147483648.0F);
    }
    case AudioFormat::S32:
      return read_s32_le(source) * (1.0F / 2147483648.0F);
    case AudioFormat::F32:
      break;
  }
  throw std::invalid_argument("unsupported integer-to-float source format");
}

inline void fill_float_audio_source(GuardedAudioBuffer& buffer, AudioFormat format,
                                    std::size_t count) {
  if (format != AudioFormat::F32 || buffer.active_bytes() != count * sizeof(float)) {
    throw std::invalid_argument("float source size or format mismatch");
  }
  for (std::size_t index = 0; index < count; ++index)
    write_float(buffer.data() + index * sizeof(float), float_anchor(index));
}

inline void convert_integer_to_float_reference(AudioFormat source_format, std::uint8_t* source,
                                               std::uint8_t* destination, std::size_t count) {
  const auto source_bytes = audio_format_bytes(source_format);
  for (std::size_t index = 0; index < count; ++index)
    write_float(destination + index * sizeof(float),
                integer_to_float_reference(source_format, source + index * source_bytes));
}

inline void convert_float_to_integer_reference(AudioFormat destination_format,
                                               const std::uint8_t* source,
                                               std::uint8_t* destination, std::size_t count) {
  for (std::size_t index = 0; index < count; ++index) {
    const float value = read_float(source + index * sizeof(float));
    const float scaled = [&] {
      switch (destination_format) {
        case AudioFormat::U8:
          return value * 128.0F;
        case AudioFormat::S16:
          return value * 32768.0F;
        case AudioFormat::S24:
        case AudioFormat::S32:
          return value * 2147483648.0F;
        case AudioFormat::F32:
          break;
      }
      throw std::invalid_argument("unsupported float destination format");
    }();

    switch (destination_format) {
      case AudioFormat::U8: {
        std::uint8_t result{};
        if (scaled >= 127.0F)
          result = 255;
        else if (scaled <= -128.0F)
          result = 0;
        else
          result = static_cast<std::uint8_t>(static_cast<std::int8_t>(scaled) + 128);
        destination[index] = result;
        break;
      }
      case AudioFormat::S16: {
        std::int16_t result{};
        if (scaled >= 32767.0F)
          result = 32767;
        else if (scaled <= -32768.0F)
          result = std::numeric_limits<std::int16_t>::min();
        else
          result = static_cast<std::int16_t>(scaled);
        write_u16_le(destination + index * 2, static_cast<std::uint16_t>(result));
        break;
      }
      case AudioFormat::S24: {
        std::int32_t result{};
        if (scaled >= 2147483647.0F)
          result = std::numeric_limits<std::int32_t>::max();
        else if (scaled <= -2147483648.0F)
          result = std::numeric_limits<std::int32_t>::min();
        else
          result = static_cast<std::int32_t>(scaled);
        write_u24_le(destination + index * 3,
                     (static_cast<std::uint32_t>(result) >> 8) & 0x00ffffffU);
        break;
      }
      case AudioFormat::S32: {
        std::int32_t result{};
        if (scaled >= 2147483647.0F)
          result = std::numeric_limits<std::int32_t>::max();
        else if (scaled <= -2147483648.0F)
          result = std::numeric_limits<std::int32_t>::min();
        else
          result = static_cast<std::int32_t>(scaled);
        write_u32_le(destination + index * 4, static_cast<std::uint32_t>(result));
        break;
      }
      case AudioFormat::F32:
        throw std::invalid_argument("unsupported float destination format");
    }
  }
}

inline ::testing::AssertionResult compare_audio_float(const GuardedAudioBuffer& expected,
                                                      const GuardedAudioBuffer& actual,
                                                      std::size_t count) {
  if (expected.active_bytes() != count * sizeof(float) ||
      actual.active_bytes() != count * sizeof(float)) {
    return ::testing::AssertionFailure() << "float active byte count mismatch";
  }
  constexpr FloatTolerance tolerance{0.0000002F, 0.000002F};
  for (std::size_t index = 0; index < count; ++index) {
    const float lhs = read_float(expected.data() + index * sizeof(float));
    const float rhs = read_float(actual.data() + index * sizeof(float));
    if (!std::isfinite(rhs)) {
      return ::testing::AssertionFailure() << "non-finite output at sample=" << index;
    }
    if (lhs == rhs) continue;
    const float difference = std::abs(lhs - rhs);
    const float limit =
        std::max(tolerance.absolute, tolerance.relative * std::max(std::abs(lhs), std::abs(rhs)));
    if (difference > limit) {
      return ::testing::AssertionFailure()
             << "sample=" << index << " expected=" << lhs << " actual=" << rhs
             << " difference=" << difference << " limit=" << limit;
    }
  }
  return ::testing::AssertionSuccess();
}

inline void run_audio_float_case(const AudioFloatCase& test_case) {
  const auto source_bytes = test_case.count * audio_format_bytes(test_case.source_format);
  const auto destination_bytes = test_case.count * audio_format_bytes(test_case.destination_format);
  GuardedAudioBuffer source(source_bytes, 64, 64, test_case.source_alignment_offset);
  GuardedAudioBuffer expected(destination_bytes, 64, 64, test_case.destination_alignment_offset);
  GuardedAudioBuffer actual(destination_bytes, 64, 64, test_case.destination_alignment_offset);

  if (test_case.source_format == AudioFormat::F32)
    fill_float_audio_source(source, test_case.source_format, test_case.count);
  else
    fill_integer_audio_source(source, test_case.source_format, test_case.count);
  const auto source_snapshot = source.snapshot_active();
  expected.fill_active(0xCD);
  actual.fill_active(0xCD);

  if (test_case.destination_format == AudioFormat::F32) {
    convert_integer_to_float_reference(test_case.source_format, source.data(), expected.data(),
                                       test_case.count);
  } else {
    convert_float_to_integer_reference(test_case.destination_format, source.data(), expected.data(),
                                       test_case.count);
  }

  test_case.variant.function(source.data(), actual.data(), static_cast<int>(test_case.count));

  if (test_case.destination_format == AudioFormat::F32) {
    EXPECT_TRUE(compare_audio_float(expected, actual, test_case.count))
        << test_case.name << " float reference mismatch for variant " << test_case.variant.name;
  } else {
    EXPECT_TRUE(compare_audio_exact(expected, actual))
        << test_case.name << " integer reference mismatch for variant " << test_case.variant.name;
    EXPECT_EQ(format_hash(hash_audio_active(actual)), test_case.expected_hash) << test_case.name;
  }
  EXPECT_TRUE(source.active_matches(source_snapshot)) << test_case.name << " modified source";
  EXPECT_TRUE(source.memory_intact()) << test_case.name << " source guard or padding corruption";
  EXPECT_TRUE(expected.memory_intact())
      << test_case.name << " reference guard or padding corruption";
  EXPECT_TRUE(actual.memory_intact()) << test_case.name << " output guard or padding corruption";
}

struct AudioTwoStageCase {
  AudioFormat source_format{};
  AudioFormat destination_format{};
  std::size_t count{};
  std::size_t source_alignment_offset{};
  std::size_t stage_alignment_offset{};
  std::size_t destination_alignment_offset{};
  Variant<AudioConvertFunction> first_variant;
  Variant<AudioConvertFunction> second_variant;
  std::string expected_hash;
  std::string name;
};

inline std::string audio_two_stage_case_name(AudioFormat source_format,
                                             AudioFormat destination_format, std::size_t count,
                                             std::size_t source_offset, std::size_t stage_offset,
                                             std::size_t destination_offset,
                                             const Variant<AudioConvertFunction>& first_variant,
                                             const Variant<AudioConvertFunction>& second_variant) {
  std::ostringstream stream;
  stream << audio_format_name(source_format) << "To" << audio_format_name(destination_format)
         << "_Count" << count << "_SrcOffset" << source_offset << "_StageOffset" << stage_offset
         << "_DstOffset" << destination_offset << "_PatternBoundaryValues_"
         << audio_variant_name(first_variant) << "_Then" << audio_variant_name(second_variant);
  return stream.str();
}

inline AudioTwoStageCase make_audio_two_stage_case(AudioFormat source_format,
                                                   AudioFormat destination_format,
                                                   std::size_t count,
                                                   Variant<AudioConvertFunction> first_variant,
                                                   Variant<AudioConvertFunction> second_variant,
                                                   std::string expected_hash = {}) {
  AudioTwoStageCase result{source_format,
                           destination_format,
                           count,
                           audio_alignment_offset(source_format),
                           audio_alignment_offset(AudioFormat::S32),
                           audio_alignment_offset(destination_format),
                           std::move(first_variant),
                           std::move(second_variant),
                           std::move(expected_hash),
                           {}};
  result.name = audio_two_stage_case_name(
      result.source_format, result.destination_format, result.count, result.source_alignment_offset,
      result.stage_alignment_offset, result.destination_alignment_offset, result.first_variant,
      result.second_variant);
  return result;
}

inline void PrintTo(const AudioTwoStageCase& test_case, std::ostream* stream) {
  *stream << test_case.name;
}

inline void run_audio_two_stage_case(const AudioTwoStageCase& test_case) {
  const auto source_bytes = test_case.count * audio_format_bytes(test_case.source_format);
  const auto stage_bytes = test_case.count * audio_format_bytes(AudioFormat::S32);
  const auto destination_bytes = test_case.count * audio_format_bytes(test_case.destination_format);
  GuardedAudioBuffer source(source_bytes, 64, 64, test_case.source_alignment_offset);
  GuardedAudioBuffer expected_stage(stage_bytes, 64, 64, test_case.stage_alignment_offset);
  GuardedAudioBuffer expected(destination_bytes, 64, 64, test_case.destination_alignment_offset);
  GuardedAudioBuffer working(stage_bytes, 64, 64, test_case.stage_alignment_offset);
  GuardedAudioBuffer actual(destination_bytes, 64, 64, test_case.destination_alignment_offset);

  if (test_case.source_format == AudioFormat::F32)
    fill_float_audio_source(source, test_case.source_format, test_case.count);
  else
    fill_integer_audio_source(source, test_case.source_format, test_case.count);
  const auto source_snapshot = source.snapshot_active();
  expected_stage.fill_active(0xCD);
  expected.fill_active(0xCD);
  working.fill_active(0xCD);
  actual.fill_active(0xCD);

  if (test_case.source_format == AudioFormat::F32 &&
      test_case.destination_format == AudioFormat::S24) {
    convert_float_to_integer_reference(AudioFormat::S32, source.data(), expected_stage.data(),
                                       test_case.count);
    convert_integer_reference(AudioFormat::S32, AudioFormat::S24, expected_stage.data(),
                              expected.data(), test_case.count);
    std::memcpy(working.data(), source.data(), source_bytes);

    test_case.first_variant.function(working.data(), working.data(),
                                     static_cast<int>(test_case.count));
    EXPECT_TRUE(compare_audio_exact(expected_stage, working))
        << test_case.name << " first-stage reference mismatch for " << test_case.first_variant.name;
    test_case.second_variant.function(working.data(), actual.data(),
                                      static_cast<int>(test_case.count));
    EXPECT_TRUE(compare_audio_exact(expected, actual))
        << test_case.name << " second-stage reference mismatch for "
        << test_case.second_variant.name;
    if (!test_case.expected_hash.empty()) {
      EXPECT_EQ(format_hash(hash_audio_active(actual)), test_case.expected_hash) << test_case.name;
    }
  } else if (test_case.source_format == AudioFormat::S24 &&
             test_case.destination_format == AudioFormat::F32) {
    convert_integer_reference(AudioFormat::S24, AudioFormat::S32, source.data(),
                              expected_stage.data(), test_case.count);
    convert_integer_to_float_reference(AudioFormat::S32, expected_stage.data(), expected.data(),
                                       test_case.count);

    test_case.first_variant.function(source.data(), working.data(),
                                     static_cast<int>(test_case.count));
    EXPECT_TRUE(compare_audio_exact(expected_stage, working))
        << test_case.name << " first-stage reference mismatch for " << test_case.first_variant.name;
    test_case.second_variant.function(working.data(), working.data(),
                                      static_cast<int>(test_case.count));
    EXPECT_TRUE(compare_audio_float(expected, working, test_case.count))
        << test_case.name << " second-stage reference mismatch for "
        << test_case.second_variant.name;
  } else {
    throw std::invalid_argument("unsupported audio two-stage conversion");
  }

  EXPECT_TRUE(source.active_matches(source_snapshot)) << test_case.name << " modified source";
  EXPECT_TRUE(source.memory_intact()) << test_case.name << " source guard or padding corruption";
  EXPECT_TRUE(expected_stage.memory_intact())
      << test_case.name << " intermediate reference guard or padding corruption";
  EXPECT_TRUE(expected.memory_intact())
      << test_case.name << " reference guard or padding corruption";
  EXPECT_TRUE(working.memory_intact()) << test_case.name << " working guard or padding corruption";
  EXPECT_TRUE(actual.memory_intact()) << test_case.name << " output guard or padding corruption";
}

}  // namespace avsut::test
