#include "audio_convert/audio_convert.h"
#include "convert_audio_test_helpers.h"
#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <vector>

TEST(AudioConvertApi, RejectsInvalidRoutesAndTargets) {
  EXPECT_EQ(ac_sample_bytes(0), 0);
  EXPECT_EQ(ac_sample_bytes(123), 0);
  for (int f = AC_U8; f <= AC_F32; ++f) {
    EXPECT_EQ(ac_get_converter(f, f, AC_TARGET_NATIVE), nullptr);
    EXPECT_EQ(ac_get_converter(0, f, AC_TARGET_C), nullptr);
    EXPECT_EQ(ac_get_converter(f, 123, AC_TARGET_C), nullptr);
  }
  EXPECT_EQ(ac_get_converter(AC_U8, AC_S16, -2), nullptr);
  EXPECT_EQ(ac_get_converter(AC_U8, AC_S16, 3), nullptr);
  EXPECT_EQ(ac_get_converter(AC_U8, AC_S16, INT64_C(1) << 62), nullptr);
  EXPECT_EQ(ac_choose_target(0), AC_TARGET_C);
  EXPECT_EQ(ac_supported_targets() & ~ac_compiled_targets(), 0);
  const int64_t target = ac_choose_target(AC_TARGET_NATIVE);
  EXPECT_EQ(ac_get_converter(AC_U8, AC_S16, target),
            ac_get_converter(AC_U8, AC_S16, AC_TARGET_NATIVE));
}

TEST(AudioConvertApi, AllTwentyRoutesPreserveInputAndMatchScalar) {
  using namespace avsut::test;
  for (int src = AC_U8; src <= AC_F32; ++src) {
    for (int dst = AC_U8; dst <= AC_F32; ++dst) {
      if (src == dst) continue;
      for (int n : {0, 1, 15, 16, 17, 255, 256, 257, 4097}) {
        SCOPED_TRACE(::testing::Message() << src << " -> " << dst << " count " << n);
        auto c = ac_get_converter(src, dst, AC_TARGET_C);
        auto native = ac_get_converter(src, dst, AC_TARGET_NATIVE);
        ASSERT_NE(c, nullptr);
        ASSERT_NE(native, nullptr);
        std::vector<uint32_t> input(n + 1);
        for (int i = 0; i <= n; ++i) input[i] = uint32_t(i) * 2654435761u;
        if (src == AC_F32) {
          const uint32_t edges[] = {0, 0x80000000u, 1, 0x7f800000u, 0xff800000u,
            0x7fc00000u, 0x7f800001u, 0x3f800000u, 0xbf800000u, 0x3f7fffffu,
            0xbf7fffffu, 0xb0000001u, 0xb0000000u, 0xafffffffu};
          for (int i = 0; i <= n; i += 2) input[i] = edges[(i / 2) % 14];
        }
        const auto original = input;
        GuardedAudioBuffer expected(size_t(n) * ac_sample_bytes(dst));
        GuardedAudioBuffer actual(size_t(n) * ac_sample_bytes(dst));
        c(input.data(), expected.data(), n);
        EXPECT_EQ(input, original);
        native(input.data(), actual.data(), n);
        EXPECT_EQ(input, original);
        EXPECT_EQ(std::memcmp(expected.data(), actual.data(), size_t(n) * ac_sample_bytes(dst)), 0);
        EXPECT_TRUE(expected.memory_intact());
        EXPECT_TRUE(actual.memory_intact());
        c(nullptr, nullptr, 0);
        native(nullptr, nullptr, 0);
        c(nullptr, nullptr, -1);
        native(nullptr, nullptr, -1);
      }
    }
  }
}
