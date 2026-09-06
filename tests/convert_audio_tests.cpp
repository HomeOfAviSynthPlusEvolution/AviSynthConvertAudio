#include <gtest/gtest.h>

#include "convert_audio_test_helpers.h"
#include "kernels_highway.h"

#include "audio_convert/audio_convert.h"



#include <array>
#include <algorithm>
#include <cmath>
#include <vector>
#include <memory>
#include <limits>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#endif


namespace avsut::test {
namespace {

inline int to_sample_type(AudioFormat format) {
  switch (format) {
    case AudioFormat::U8: return AC_U8;
    case AudioFormat::S16: return AC_S16;
    case AudioFormat::S24: return AC_S24;
    case AudioFormat::S32: return AC_S32;
    case AudioFormat::F32: return AC_F32;
  }
  return 0;
}

template <typename Function>
void add_integer_variants(std::vector<AudioIntegerCase>& cases, AudioFormat source,
                          AudioFormat destination, std::size_t count, const char* expected_hash,
                          Function c_function) {
  cases.push_back(make_audio_integer_case(
      source, destination, count,
      Variant<AudioConvertFunction>{"c", c_function}, expected_hash));

  const int src_type = to_sample_type(source);
  const int dst_type = to_sample_type(destination);
  const auto hwy_c = audio_convert::ResolveHighwayAudioConvert(src_type, dst_type, 0);
  const auto hwy_native = audio_convert::ResolveHighwayAudioConvert(src_type, dst_type, ~0);

  cases.push_back(make_audio_integer_case(
      source, destination, count,
      Variant<AudioConvertFunction>{"hwy_c", hwy_c}, expected_hash));
  cases.push_back(make_audio_integer_case(
      source, destination, count,
      Variant<AudioConvertFunction>{"hwy_native", hwy_native}, expected_hash));
}

TEST(AudioHighwayDispatch, AllRoutesResolveToCOrNative) {
  struct Route { int src; int dst; convert_proc scalar; };
  const Route routes[] = {
    {AC_U8,  AC_S16, convert8To16},
    {AC_S16, AC_U8,  convert16To8},
    {AC_U8,  AC_S32, convert8To32},
    {AC_S32, AC_U8,  convert32To8},
    {AC_S16, AC_S32, convert16To32},
    {AC_S32, AC_S16, convert32To16},
    {AC_S32, AC_S24, convert32To24},
    {AC_S24, AC_S32, convert24To32},
    {AC_S24, AC_S16, convert24To16},
    {AC_S16, AC_S24, convert16To24},
    {AC_S24, AC_U8,  convert24To8},
    {AC_U8,  AC_S24, convert8To24},
    {AC_U8,  AC_F32, convert8ToFLT},
    {AC_F32, AC_U8, convertFLTTo8},
    {AC_S16, AC_F32, convert16ToFLT},
    {AC_F32, AC_S16, convertFLTTo16},
    {AC_S32, AC_F32, convert32ToFLT},
    {AC_F32, AC_S32, convertFLTTo32},
  };
  const auto target = audio_convert::GetHighwayAudioConvertChosenTarget(~int64_t{0});
  EXPECT_EQ(audio_convert::GetHighwayAudioConvertChosenTarget(~0), target);
  EXPECT_EQ(audio_convert::GetHighwayAudioConvertChosenTarget(0), 0);
  for (const auto& route : routes) {
    SCOPED_TRACE(::testing::Message() << route.src << " -> " << route.dst);
    EXPECT_EQ(audio_convert::ResolveHighwayAudioConvert(route.src, route.dst, 0), route.scalar);
    const auto native = audio_convert::ResolveHighwayAudioConvert(route.src, route.dst, ~0);
    ASSERT_NE(native, nullptr);
    if (target == 0) {
      EXPECT_EQ(native, route.scalar);
    } else {
      EXPECT_NE(native, route.scalar);
      EXPECT_EQ(native, audio_convert::ResolveHighwayAudioConvertForTarget(route.src, route.dst, target));
    }
  }

  // Direct float routes: S24 <-> F32
  // The library owns the complete ordinary C fallback for both routes.
  EXPECT_EQ(audio_convert::ResolveHighwayAudioConvert(AC_S24, AC_F32, 0), convert24ToFLT);
  EXPECT_EQ(audio_convert::ResolveHighwayAudioConvert(AC_F32, AC_S24, 0), convertFLTTo24);
  const auto s24_to_f32_native = audio_convert::ResolveHighwayAudioConvert(AC_S24, AC_F32, ~0);
  const auto f32_to_s24_native = audio_convert::ResolveHighwayAudioConvert(AC_F32, AC_S24, ~0);
  if (target == 0) {
    EXPECT_EQ(s24_to_f32_native, convert24ToFLT);
    EXPECT_EQ(f32_to_s24_native, convertFLTTo24);
  } else {
    ASSERT_NE(s24_to_f32_native, nullptr);
    ASSERT_NE(f32_to_s24_native, nullptr);
    EXPECT_EQ(s24_to_f32_native, audio_convert::ResolveHighwayAudioConvertForTarget(AC_S24, AC_F32, target));
    EXPECT_EQ(f32_to_s24_native, audio_convert::ResolveHighwayAudioConvertForTarget(AC_F32, AC_S24, target));
  }

  for (int type : {AC_U8, AC_S16, AC_S24, AC_S32, AC_F32}) {
    EXPECT_FALSE(audio_convert::IsHighwayAudioConvertSupportedRoute(type, type));
    EXPECT_EQ(audio_convert::ResolveHighwayAudioConvert(type, type, ~0), nullptr);
  }
  EXPECT_EQ(audio_convert::ResolveHighwayAudioConvert(0, AC_F32, ~0), nullptr);

}

std::vector<AudioIntegerCase> audio_integer_cases() {
  constexpr std::array<std::size_t, 6> counts{7, 8, 9, 15, 16, 17};
  constexpr std::array<const char*, 6> s32_to_s16{"72636ca0711e509d", "bc0055e922cca5c1",
                                                  "dddf915c0a598053", "7f85c2db07160a03",
                                                  "8583483f9c7f777d", "236d8ef6157ac922"};
  constexpr std::array<const char*, 6> s16_to_s32{"7f585b802d4eefde", "a709f5d101dab569",
                                                  "8493cdc5eac29749", "f58c1429ba6987ee",
                                                  "e75c50f543851658", "5fb431b11e75eca8"};
  constexpr std::array<const char*, 6> s32_to_u8{"2fa42ce33b9b9690", "26631036a0122595",
                                                 "5089246ee560804b", "94e50bd25b3fd8ca",
                                                 "4e7769dc192a2c6d", "20fa999e5e2c8324"};
  constexpr std::array<const char*, 6> u8_to_s32{"1365282cea4e33d3", "f53ba3e2b796b5a5",
                                                 "9e6d204fccb2fb77", "1e543f3519614fba",
                                                 "f2e1a143da816fa7", "5bd5391fc2326e74"};
  constexpr std::array<const char*, 6> s16_to_u8{"6d140c77ecf217a9", "825796fed5f9a30b",
                                                 "8fea9faa63e457e4", "f5cc9a5828084c2c",
                                                 "a9109668148bb410", "9b3d2c9927677728"};
  constexpr std::array<const char*, 6> u8_to_s16{"5f8296cbe98967a7", "b827603c85c112e3",
                                                 "256c7f8cff50d066", "65e83ea200c3bb22",
                                                 "e9f8dbb9828874e2", "730ae7733badca4d"};
  std::vector<AudioIntegerCase> cases;
  for (std::size_t index = 0; index < counts.size(); ++index) {
    const auto count = counts[index];
    add_integer_variants(cases, AudioFormat::S32, AudioFormat::S16, count, s32_to_s16[index], convert32To16);
    add_integer_variants(cases, AudioFormat::S16, AudioFormat::S32, count, s16_to_s32[index], convert16To32);
    add_integer_variants(cases, AudioFormat::S32, AudioFormat::U8, count, s32_to_u8[index], convert32To8);
    add_integer_variants(cases, AudioFormat::U8, AudioFormat::S32, count, u8_to_s32[index], convert8To32);
    add_integer_variants(cases, AudioFormat::S16, AudioFormat::U8, count, s16_to_u8[index], convert16To8);
    add_integer_variants(cases, AudioFormat::U8, AudioFormat::S16, count, u8_to_s16[index], convert8To16);
  }

  constexpr std::array<std::size_t, 8> edge_counts{0, 1, 2, 3, 31, 32, 33, 64};
  for (const auto count : edge_counts) {
    add_integer_variants(cases, AudioFormat::S32, AudioFormat::S16, count, "", convert32To16);
    add_integer_variants(cases, AudioFormat::S16, AudioFormat::S32, count, "", convert16To32);
    add_integer_variants(cases, AudioFormat::S32, AudioFormat::U8, count, "", convert32To8);
    add_integer_variants(cases, AudioFormat::U8, AudioFormat::S32, count, "", convert8To32);
    add_integer_variants(cases, AudioFormat::S16, AudioFormat::U8, count, "", convert16To8);
    add_integer_variants(cases, AudioFormat::U8, AudioFormat::S16, count, "", convert8To16);
  }

  return cases;
}

template <typename Function>
void add_packed_24_variants(std::vector<AudioIntegerCase>& cases, AudioFormat source,
                            AudioFormat destination, std::size_t count, const char* expected_hash,
                            Function c_function) {
  cases.push_back(make_audio_integer_case(
      source, destination, count,
      Variant<AudioConvertFunction>{"c", c_function}, expected_hash));

  const int src_type = to_sample_type(source);
  const int dst_type = to_sample_type(destination);
  const auto hwy_c = audio_convert::ResolveHighwayAudioConvert(src_type, dst_type, 0);
  const auto hwy_native = audio_convert::ResolveHighwayAudioConvert(src_type, dst_type, ~0);

  cases.push_back(make_audio_integer_case(
      source, destination, count,
      Variant<AudioConvertFunction>{"hwy_c", hwy_c}, expected_hash));
  cases.push_back(make_audio_integer_case(
      source, destination, count,
      Variant<AudioConvertFunction>{"hwy_native", hwy_native}, expected_hash));
}

std::vector<AudioIntegerCase> audio_packed_24_cases() {
  constexpr std::array<std::size_t, 3> counts{15, 16, 17};
  constexpr std::array<const char*, 3> s32_to_s24{"56c34a4a40d2b71f", "ce35878a8c7ba570",
                                                  "27a959747be110f8"};
  constexpr std::array<const char*, 3> s24_to_s32{"544236777f760a57", "1a6fdfcb7259f9d6",
                                                  "4c7abe1834a5cfd4"};
  constexpr std::array<const char*, 3> s24_to_s16{"b9a412f8dd275583", "a5f6058831cbef94",
                                                  "eb36bfad072941c8"};
  constexpr std::array<const char*, 3> s16_to_s24{"9d6f6a2f95056b94", "d615b9752abe3b7f",
                                                  "508d07839f9959a4"};
  constexpr std::array<const char*, 3> s24_to_u8{"be5a748b497f8a72", "fd804383de2f7954",
                                                 "7b8589ad51b4c7bc"};
  constexpr std::array<const char*, 3> u8_to_s24{"14ad13c337e3e265", "3affb442fd5a2bf7",
                                                 "a7b10fbd795a1245"};
  std::vector<AudioIntegerCase> cases;
  for (std::size_t index = 0; index < counts.size(); ++index) {
    const auto count = counts[index];
    add_packed_24_variants(cases, AudioFormat::S32, AudioFormat::S24, count, s32_to_s24[index], convert32To24);
    add_packed_24_variants(cases, AudioFormat::S24, AudioFormat::S32, count, s24_to_s32[index], convert24To32);
    add_packed_24_variants(cases, AudioFormat::S24, AudioFormat::S16, count, s24_to_s16[index], convert24To16);
    add_packed_24_variants(cases, AudioFormat::S16, AudioFormat::S24, count, s16_to_s24[index], convert16To24);
    add_packed_24_variants(cases, AudioFormat::S24, AudioFormat::U8,  count, s24_to_u8[index],  convert24To8);
    add_packed_24_variants(cases, AudioFormat::U8,  AudioFormat::S24, count, u8_to_s24[index],  convert8To24);
  }

  constexpr std::array<std::size_t, 8> edge_counts{0, 1, 2, 3, 31, 32, 33, 64};
  for (const auto count : edge_counts) {
    add_packed_24_variants(cases, AudioFormat::S32, AudioFormat::S24, count, "", convert32To24);
    add_packed_24_variants(cases, AudioFormat::S24, AudioFormat::S32, count, "", convert24To32);
    add_packed_24_variants(cases, AudioFormat::S24, AudioFormat::S16, count, "", convert24To16);
    add_packed_24_variants(cases, AudioFormat::S16, AudioFormat::S24, count, "", convert16To24);
    add_packed_24_variants(cases, AudioFormat::S24, AudioFormat::U8,  count, "", convert24To8);
    add_packed_24_variants(cases, AudioFormat::U8,  AudioFormat::S24, count, "", convert8To24);
  }

  return cases;
}

void add_audio_float_variants(std::vector<AudioFloatCase>& cases, AudioFormat source,
                              AudioFormat destination, std::size_t count,
                              AudioConvertFunction c_function, const char* expected_hash = "") {
  cases.push_back(make_audio_float_case(source, destination, count,
      Variant<AudioConvertFunction>{"c", c_function}, expected_hash));
  const auto native = audio_convert::ResolveHighwayAudioConvert(
      to_sample_type(source), to_sample_type(destination), ~0);
  cases.push_back(make_audio_float_case(source, destination, count,
      Variant<AudioConvertFunction>{"hwy_native", native}, expected_hash));
}

std::vector<AudioFloatCase> audio_float_cases() {
  constexpr std::array<std::size_t, 9> counts{3, 4, 5, 7, 8, 9, 15, 16, 17};
  constexpr std::array<const char*, 9> f32_to_u8{
      "6f15e63fb37c575a", "3a28021824265c2d", "66b1cfc7b7f25862",
      "561e5d7d5e7bf725", "83f0e9d7f6ca57a3", "7df3b53ce739e7f9",
      "70f7678b55537c79", "d26cc00c361369f4", "c52344360f8996c9"};
  constexpr std::array<const char*, 9> f32_to_s16{
      "9fdf2c12fa9cda45", "225b4a1ac27203ae", "75c6b5dfe0f9537b",
      "fa2dbc74fa708fdd", "33e13965e140038a", "0e971aa9e1527ecf",
      "ffcec376b852647e", "853336eb38ef2a2a", "35972615fb81e5e4"};
  constexpr std::array<const char*, 9> f32_to_s32{
      "8025870c1821c9ed", "8fd4e841a1714e07", "ce12e02dabf378d3",
      "46523c3304543fec", "6c949caf27523637", "b35758abba35b489",
      "49ff03bf11065eaf", "d4c900a729fc2049", "17f9f15dbbd30f7f"};
  constexpr std::array<const char*, 9> f32_to_s24{
      "3856194f641f6364", "130bd615dd56e05c", "1ace15baf02dfc76",
      "36b7e16d7e866f28", "29a5e77048f76506", "7d10b3c6122283e0",
      "f31a61d307ad35b6", "fde8884581bbcf19", "36cfd1c0483386f7"};
  std::vector<AudioFloatCase> cases;
  for (std::size_t index = 0; index < counts.size(); ++index) {
    const auto count = counts[index];
    add_audio_float_variants(cases, AudioFormat::U8, AudioFormat::F32, count, convert8ToFLT);
    add_audio_float_variants(cases, AudioFormat::F32, AudioFormat::U8, count, convertFLTTo8, f32_to_u8[index]);
    add_audio_float_variants(cases, AudioFormat::S16, AudioFormat::F32, count, convert16ToFLT);
    add_audio_float_variants(cases, AudioFormat::F32, AudioFormat::S16, count, convertFLTTo16, f32_to_s16[index]);
    add_audio_float_variants(cases, AudioFormat::S32, AudioFormat::F32, count, convert32ToFLT);
    add_audio_float_variants(cases, AudioFormat::F32, AudioFormat::S32, count, convertFLTTo32, f32_to_s32[index]);

    const auto s24_to_f32_native = audio_convert::ResolveHighwayAudioConvert(AC_S24, AC_F32, ~0);
    if (s24_to_f32_native) {
      cases.push_back(make_audio_float_case(
          AudioFormat::S24, AudioFormat::F32, count,
          Variant<AudioConvertFunction>{"hwy_native", s24_to_f32_native}));
    }
    const auto f32_to_s24_native = audio_convert::ResolveHighwayAudioConvert(AC_F32, AC_S24, ~0);
    if (f32_to_s24_native) {
      cases.push_back(make_audio_float_case(
          AudioFormat::F32, AudioFormat::S24, count,
          Variant<AudioConvertFunction>{"hwy_native", f32_to_s24_native},
          f32_to_s24[index]));
    }
  }
  return cases;
}

std::vector<AudioTwoStageCase> audio_two_stage_cases() {
  constexpr std::array<std::size_t, 6> counts{3, 4, 5, 15, 16, 17};
  constexpr std::array<const char*, 6> f32_to_s24_hashes{"3856194f641f6364", "130bd615dd56e05c",
                                                         "1ace15baf02dfc76", "f31a61d307ad35b6",
                                                         "fde8884581bbcf19", "36cfd1c0483386f7"};
  // Only ordinary C uses the two-stage path in production.
  const std::array<Variant<AudioConvertFunction>, 1> float_to_s32{
      Variant<AudioConvertFunction>{"c", convertFLTTo32}};
  const std::array<Variant<AudioConvertFunction>, 1> s32_to_s24{
      Variant<AudioConvertFunction>{"c", convert32To24}};
  const std::array<Variant<AudioConvertFunction>, 1> s24_to_s32{
      Variant<AudioConvertFunction>{"c", convert24To32}};
  const std::array<Variant<AudioConvertFunction>, 1> s32_to_float{
      Variant<AudioConvertFunction>{"c", convert32ToFLT}};

  std::vector<AudioTwoStageCase> cases;
  for (std::size_t index = 0; index < counts.size(); ++index) {
    const auto count = counts[index];
    for (const auto& first : float_to_s32) {
      for (const auto& second : s32_to_s24) {
        if (!first.function || !second.function) continue;
        cases.push_back(make_audio_two_stage_case(AudioFormat::F32, AudioFormat::S24, count, first,
                                                  second, f32_to_s24_hashes[index]));
      }
    }
    for (const auto& first : s24_to_s32) {
      for (const auto& second : s32_to_float) {
        if (!first.function || !second.function) continue;
        cases.push_back(
            make_audio_two_stage_case(AudioFormat::S24, AudioFormat::F32, count, first, second));
      }
    }
  }
  return cases;
}

class AudioIntegerKernels : public ::testing::TestWithParam<AudioIntegerCase> {};

TEST_P(AudioIntegerKernels, MatchesIndependentIntegerReference) {
  const auto& test_case = GetParam();
  ASSERT_NE(test_case.variant.function, nullptr);
  run_audio_integer_case(test_case);
}

INSTANTIATE_TEST_SUITE_P(Kernels, AudioIntegerKernels, ::testing::ValuesIn(audio_integer_cases()),
                         [](const ::testing::TestParamInfo<AudioIntegerCase>& info) {
                           return info.param.name;
                         });

class AudioPacked24Kernels : public ::testing::TestWithParam<AudioIntegerCase> {};

TEST_P(AudioPacked24Kernels, MatchesIndependentPacked24Reference) {
  const auto& test_case = GetParam();
  run_audio_integer_case(test_case);
}

INSTANTIATE_TEST_SUITE_P(Packed24, AudioPacked24Kernels,
                         ::testing::ValuesIn(audio_packed_24_cases()),
                         [](const ::testing::TestParamInfo<AudioIntegerCase>& info) {
                           return info.param.name;
                         });

class AudioFloatKernels : public ::testing::TestWithParam<AudioFloatCase> {};

TEST_P(AudioFloatKernels, MatchesIndependentFloatConversionReference) {
  const auto& test_case = GetParam();
  run_audio_float_case(test_case);
}

INSTANTIATE_TEST_SUITE_P(Float, AudioFloatKernels, ::testing::ValuesIn(audio_float_cases()),
                         [](const ::testing::TestParamInfo<AudioFloatCase>& info) {
                           return info.param.name;
                         });

class AudioTwoStageKernels : public ::testing::TestWithParam<AudioTwoStageCase> {};

TEST_P(AudioTwoStageKernels, MatchesConvertAudioTwoStageReference) {
  const auto& test_case = GetParam();
  run_audio_two_stage_case(test_case);
}

INSTANTIATE_TEST_SUITE_P(TwoStage, AudioTwoStageKernels,
                         ::testing::ValuesIn(audio_two_stage_cases()),
                         [](const ::testing::TestParamInfo<AudioTwoStageCase>& info) {
                           return info.param.name;
                         });


TEST(AudioPacked24Float, MatchesComposedCAtQuantizationBoundaries) {
  const auto convert = audio_convert::ResolveHighwayAudioConvert(AC_F32, AC_S24, ~0);
  if (!convert) GTEST_SKIP() << "No SIMD target on this host";
  const float inf = std::numeric_limits<float>::infinity();
  std::vector<float> values{0.0f, -0.0f, inf, -inf,
      std::numeric_limits<float>::quiet_NaN(), -std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::denorm_min(), -std::numeric_limits<float>::denorm_min()};
  for (float boundary : {-1.0f, -0x1p-23f, -0x1p-31f, 0x1p-31f, 0x1p-23f, 1.0f}) {
    values.push_back(std::nextafter(boundary, -inf));
    values.push_back(boundary);
    values.push_back(std::nextafter(boundary, inf));
  }
  const auto seeds = values;
  while (values.size() < 65) values.push_back(seeds[values.size() % seeds.size()]);
  // Rotate each value through vector blocks and the scalar tail.
  for (size_t rotation = 0; rotation < values.size(); ++rotation) {
    std::rotate(values.begin(), values.begin() + 1, values.end());
    std::vector<int32_t> intermediate(values.size());
    std::vector<uint8_t> expected(values.size() * 3), actual(expected.size());
    convertFLTTo32(values.data(), intermediate.data(), int(values.size()));
    convert32To24(intermediate.data(), expected.data(), int(values.size()));
    convert(values.data(), actual.data(), int(values.size()));
    EXPECT_EQ(actual, expected) << "rotation " << rotation;
  }
}

TEST(AudioFloatConversion, PreservesQuantizationAtBoundariesAndRandomFloatBits) {
  const float inf = std::numeric_limits<float>::infinity();
  struct Route { int format; size_t bytes; convert_proc scalar; float scale; };
  for (const auto& route : {Route{AC_U8, 1, convertFLTTo8, 128.0f},
                            Route{AC_S16, 2, convertFLTTo16, 32768.0f},
                            Route{AC_S32, 4, convertFLTTo32, 2147483648.0f}}) {
    const auto native = audio_convert::ResolveHighwayAudioConvert(AC_F32, route.format, ~0);
    ASSERT_NE(native, nullptr);
    std::vector<float> seeds{0.0f, -0.0f, inf, -inf,
        std::numeric_limits<float>::denorm_min(), -std::numeric_limits<float>::denorm_min()};
    for (float boundary : {-1.0f, -1.0f / route.scale, 1.0f / route.scale,
                           (route.scale - 1.0f) / route.scale, 1.0f}) {
      seeds.push_back(std::nextafter(boundary, -inf));
      seeds.push_back(boundary);
      seeds.push_back(std::nextafter(boundary, inf));
    }
    for (uint32_t bits : {0x7fc00001u, 0xffc00001u, 0x7f800001u, 0xff800001u}) {
      float value;
      std::memcpy(&value, &bits, sizeof(value));
      seeds.push_back(value);
    }
    for (int count : {0, 1, 3, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65, 257, 4097}) {
      SCOPED_TRACE(::testing::Message() << route.format << " count=" << count);
      for (size_t offset = 0; offset < seeds.size(); ++offset) {
        std::vector<float> input(count);
        for (int i = 0; i < count; ++i) {
          if (i % 2 == 0) input[i] = seeds[(i / 2 + offset) % seeds.size()];
          else {
            const uint32_t bits = uint32_t(i + offset * 4099) * 2654435761u;
            std::memcpy(&input[i], &bits, sizeof(bits));
          }
        }
        const auto before = input;
        std::vector<uint8_t> expected(count * route.bytes), actual(expected.size());
        route.scalar(input.data(), expected.data(), count);
        native(input.data(), actual.data(), count);
        EXPECT_EQ(actual, expected) << "offset " << offset;
        if (count) EXPECT_EQ(std::memcmp(input.data(), before.data(), count * sizeof(float)), 0);
      }
    }
  }
}

TEST(AudioFloatConversion, NormalizesIntegerDomainExactly) {
  struct Route { int format; size_t bytes; convert_proc scalar; };
  for (const auto& route : {Route{AC_U8, 1, convert8ToFLT},
                            Route{AC_S16, 2, convert16ToFLT},
                            Route{AC_S32, 4, convert32ToFLT}}) {
    const auto native = audio_convert::ResolveHighwayAudioConvert(route.format, AC_F32, ~0);
    ASSERT_NE(native, nullptr);
    // Exhaust U8 and S16; use deterministic S32 bit patterns including endpoints.
    const int count = route.bytes == 1 ? 256 : 65536;
    std::vector<uint8_t> input(count * route.bytes);
    for (int i = 0; i < count; ++i) {
      if (route.bytes == 1) input[i] = uint8_t(i);
      else if (route.bytes == 2) write_u16_le(input.data() + i * 2, uint16_t(i));
      else write_u32_le(input.data() + i * 4, uint32_t(i) * 2654435761u);
    }
    if (route.bytes == 4) {
      write_u32_le(input.data(), 0x7fffffffu);
      write_u32_le(input.data() + 4, 0x80000000u);
      write_u32_le(input.data() + 8, 0xffffffffu);
    }
    std::vector<float> expected(count), actual(count);
    route.scalar(input.data(), expected.data(), count);
    native(input.data(), actual.data(), count);
    EXPECT_EQ(std::memcmp(actual.data(), expected.data(), count * sizeof(float)), 0) << route.format;
  }
}

#ifdef _WIN32
TEST(AudioConversionBoundary, NoAccessPastInputOrOutputBufferEnd) {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  const size_t page_size = si.dwPageSize;
  const auto release = [](uint8_t* p) { if (p) VirtualFree(p, 0, MEM_RELEASE); };
  using Allocation = std::unique_ptr<uint8_t, decltype(release)>;
  Allocation input(static_cast<uint8_t*>(VirtualAlloc(nullptr, page_size * 2,
      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)), release);
  Allocation output(static_cast<uint8_t*>(VirtualAlloc(nullptr, page_size * 2,
      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)), release);
  ASSERT_NE(input, nullptr);
  ASSERT_NE(output, nullptr);
  DWORD old_protect;
  ASSERT_TRUE(VirtualProtect(input.get() + page_size, page_size, PAGE_NOACCESS, &old_protect));
  ASSERT_TRUE(VirtualProtect(output.get() + page_size, page_size, PAGE_NOACCESS, &old_protect));
  const int formats[] = {AC_U8, AC_S16, AC_S24, AC_S32, AC_F32};
  const auto width = [](int format) -> size_t {
    return format == AC_U8 ? 1 : format == AC_S16 ? 2 : format == AC_S24 ? 3 : 4;
  };
  for (int src : formats) {
    for (int dst : formats) {
      if (src == dst) continue;
      const auto convert = audio_convert::ResolveHighwayAudioConvert(src, dst, ~0);
      if (!convert && audio_convert::GetHighwayAudioConvertChosenTarget(~0) == 0 &&
          ((src == AC_S24 && dst == AC_F32) || (src == AC_F32 && dst == AC_S24))) continue;
      ASSERT_NE(convert, nullptr);
      for (size_t count : {0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129, 257}) {
        SCOPED_TRACE(::testing::Message() << src << " -> " << dst << " count=" << count);
        auto* in = input.get() + page_size - count * width(src);
        auto* out = output.get() + page_size - count * width(dst);
        std::memset(in, 0x5A, count * width(src));
        convert(in, out, int(count));
      }
    }
  }
}
#endif


}  // namespace
}  // namespace avsut::test
