#include "audio_convert/audio_convert.h"
#include <stddef.h>

int main(void) {
  const unsigned char input[] = {0, 128, 255};
  const int16_t expected[] = {-32768, 0, 32512};
  int16_t output[3];
  const int64_t targets[] = {AC_TARGET_C, AC_TARGET_NATIVE};
  for (int k = 0; k < 2; ++k) {
    ac_convert_fn fn = ac_get_converter(AC_U8, AC_S16, targets[k]);
    if (!fn) return 1;
    fn(input, output, 3);
    for (int i = 0; i < 3; ++i) if (output[i] != expected[i]) return 2;
    fn(NULL, NULL, 0);
  }
  return ac_get_converter(AC_U8, AC_U8, AC_TARGET_NATIVE) != NULL;
}
