#include "audio_convert/audio_convert.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

int main() {
  constexpr int count = 4096;
  constexpr int repeats = 20000;
  uint64_t checksum = 0;
  const char* names[] = {"invalid", "U8", "S16", "S24", "S32", "F32"};
  std::printf("native_target,%lld\n", static_cast<long long>(ac_choose_target(AC_TARGET_NATIVE)));
  std::puts("source,destination,samples,c_ns,native_ns,new_over_c");
  for (int src = AC_U8; src <= AC_F32; ++src) for (int dst = AC_U8; dst <= AC_F32; ++dst) {
    if (src == dst) continue;
    std::vector<uint32_t> input(count), output(count), expected(count);
    for (int i = 0; i < count; ++i) {
      input[i] = uint32_t(i) * 2654435761u;
      if (src == AC_F32) {
        float f = float((i * 73) % 257 - 128) / 128.0f;
        std::memcpy(&input[i], &f, sizeof(f));
      }
    }
    ac_convert_fn functions[] = {ac_get_converter(src, dst, AC_TARGET_C),
                                ac_get_converter(src, dst, AC_TARGET_NATIVE)};
    if (!functions[0] || !functions[1]) return 1;
    functions[0](input.data(), expected.data(), count);
    functions[1](input.data(), output.data(), count);
    if (std::memcmp(expected.data(), output.data(), count * ac_sample_bytes(dst))) return 2;
    std::array<double, 7> times[2];
    for (int round = 0; round < 7; ++round) for (int step = 0; step < 2; ++step) {
      const int k = (round + step) % 2;
      for (int i = 0; i < 100; ++i) functions[k](input.data(), output.data(), count);
      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < repeats; ++i) functions[k](input.data(), output.data(), count);
      times[k][round] = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count() / repeats;
      checksum += output[count * ac_sample_bytes(dst) / 4 - 1];
    }
    for (auto& t : times) std::sort(t.begin(), t.end());
    std::printf("%s,%s,%d,%.3f,%.3f,%.3f\n", names[src], names[dst], count,
                times[0][3], times[1][3], times[1][3] / times[0][3]);
  }
  std::printf("checksum,%llu\n", static_cast<unsigned long long>(checksum));
}
