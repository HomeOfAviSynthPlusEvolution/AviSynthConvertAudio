// Developed in AviSynthMinus from the AviSynth audio conversion implementation.
// Licensed under GPL version 2 or later with the inherited AviSynth
// linking exception. See LICENSE.
#pragma once
#include "kernels.h"
#include <cstdint>
namespace audio_convert {
convert_proc ResolveHighwayAudioConvert(int src, int dst, int64_t allowed_targets);
convert_proc ResolveHighwayAudioConvertForTarget(int src, int dst, int64_t target);
int64_t GetHighwayAudioConvertChosenTarget(int64_t allowed_targets);
int64_t GetHighwayAudioConvertCompiledTargets();
bool IsHighwayAudioConvertSupportedRoute(int src, int dst);
}
