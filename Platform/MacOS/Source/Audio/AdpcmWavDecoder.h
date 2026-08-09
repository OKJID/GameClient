#pragma once

#include <cstddef>
#include <cstdint>

// Decode Generals IMA ADPCM WAV (format 0x0011) to interleaved signed 16-bit PCM.
// PCM WAV (format 1) is not handled here — caller keeps the existing path.
// On success: *outPcm is malloc'd (caller frees), returns true.

bool AdpcmWav_DecodeImaToPcm16(
    const uint8_t *wavData,
    size_t wavBytes,
    uint8_t **outPcm,
    uint32_t *outPcmBytes,
    uint32_t *outSampleRate,
    uint16_t *outChannels);
