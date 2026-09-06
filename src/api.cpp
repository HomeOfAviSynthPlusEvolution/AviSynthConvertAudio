// Licensed under GPL version 2 or later with the inherited AviSynth
// linking exception. See LICENSE.
#include "audio_convert/audio_convert.h"
#include "kernels_highway.h"
#include <hwy/targets.h>
#include <limits>

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559, "F32 requires IEEE 754 binary32");

extern "C" {
int ac_sample_bytes(int format) {
  switch (format) {
    case AC_U8:
      return 1;
    case AC_S16:
      return 2;
    case AC_S24:
      return 3;
    case AC_S32:
    case AC_F32:
      return 4;
    default:
      return 0;
  }
}
int64_t ac_compiled_targets(void) {
  return audio_convert::GetHighwayAudioConvertCompiledTargets();
}
int64_t ac_supported_targets(void) {
  return ac_compiled_targets() & hwy::SupportedTargets();
}
int64_t ac_choose_target(int64_t allowed_targets) {
  return audio_convert::GetHighwayAudioConvertChosenTarget(allowed_targets);
}
ac_convert_fn ac_get_converter(int src, int dst, int64_t target) {
  if (!ac_sample_bytes(src) || !ac_sample_bytes(dst) || src == dst)
    return nullptr;
  if (target == AC_TARGET_NATIVE)
    target = ac_choose_target(AC_TARGET_NATIVE);
  else if (target != AC_TARGET_C &&
           (target < 0 || (target & (target - 1)) != 0 || (ac_supported_targets() & target) == 0))
    return nullptr;
  return audio_convert::ResolveHighwayAudioConvertForTarget(src, dst, target);
}
}
