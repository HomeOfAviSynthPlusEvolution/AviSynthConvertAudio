// Developed in AviSynthMinus from the AviSynth audio conversion implementation.
// Licensed under GPL version 2 or later with the inherited AviSynth
// linking exception. See LICENSE.
#pragma once

#include <cstdint>


namespace audio_convert {

#define CONVERT_DECLARE(func) void (func)(const void *, void *, int);

typedef CONVERT_DECLARE(*convert_proc);

CONVERT_DECLARE(convert32To16);
CONVERT_DECLARE(convert16To32);
CONVERT_DECLARE(convert32To8);
CONVERT_DECLARE(convert8To32);
CONVERT_DECLARE(convert16To8);
CONVERT_DECLARE(convert8To16);
CONVERT_DECLARE(convert32To24);
CONVERT_DECLARE(convert24To32);
CONVERT_DECLARE(convert24To16);
CONVERT_DECLARE(convert16To24);
CONVERT_DECLARE(convert24To8);
CONVERT_DECLARE(convert8To24);
CONVERT_DECLARE(convert8ToFLT);
CONVERT_DECLARE(convertFLTTo8);
CONVERT_DECLARE(convert16ToFLT);
CONVERT_DECLARE(convertFLTTo16);
CONVERT_DECLARE(convert32ToFLT);
CONVERT_DECLARE(convertFLTTo32);


#undef CONVERT_DECLARE

void convert24ToFLT(const void*, void*, int);
void convertFLTTo24(const void*, void*, int);

} // namespace audio_convert
