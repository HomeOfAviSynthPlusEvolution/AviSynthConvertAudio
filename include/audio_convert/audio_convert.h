// Licensed under GPL version 2 or later with the inherited AviSynth
// linking exception. See LICENSE.
#ifndef AUDIO_CONVERT_AUDIO_CONVERT_H
#define AUDIO_CONVERT_AUDIO_CONVERT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ac_sample_format { AC_U8 = 1, AC_S16 = 2, AC_S24 = 3, AC_S32 = 4, AC_F32 = 5 };

#define AC_TARGET_C INT64_C(0)
#define AC_TARGET_NATIVE INT64_C(-1)

// count is the number of individual channel values, from 0 through INT_MAX.
// count <= 0 performs no access. Otherwise both buffers must be valid, disjoint
// and large enough for count samples. Typed samples require natural alignment;
// no SIMD alignment is required. S24 is packed little-endian; other types use
// native byte order. Conversion allocates no memory and does not throw.
typedef void (*ac_convert_fn)(const void* input, void* output, int count);

// Returns 0 for an invalid format.
int ac_sample_bytes(int format);

// Highway target masks. Neither mask includes the ordinary C fallback.
int64_t ac_compiled_targets(void);
int64_t ac_supported_targets(void);

// Highest-priority compiled, hardware-supported target allowed by the mask.
// An empty intersection returns AC_TARGET_C. Does not modify global policy.
int64_t ac_choose_target(int64_t allowed_targets);

// target is AC_TARGET_NATIVE, AC_TARGET_C, or a single Highway target bit.
// Explicit targets must be compiled and supported by this machine.
// Returns NULL for invalid/same-format routes or unsupported explicit targets.
// Returned functions remain valid for the lifetime of the linked library.
ac_convert_fn ac_get_converter(int source_format, int destination_format, int64_t target);

#ifdef __cplusplus
}
#endif
#endif
