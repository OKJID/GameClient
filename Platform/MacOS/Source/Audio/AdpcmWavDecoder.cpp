#include "AdpcmWavDecoder.h"

#include <cstdlib>
#include <cstring>

namespace {

static uint16_t rd16(const uint8_t *p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static const int kIndexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8,
};

static const int kStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

static int16_t decodeNibble(uint8_t nibble, int *pred, int *index) {
  int step = kStepTable[*index];
  int diff = step >> 3;
  if (nibble & 1) {
    diff += step >> 2;
  }
  if (nibble & 2) {
    diff += step >> 1;
  }
  if (nibble & 4) {
    diff += step;
  }
  if (nibble & 8) {
    *pred -= diff;
  } else {
    *pred += diff;
  }
  if (*pred < -32768) {
    *pred = -32768;
  } else if (*pred > 32767) {
    *pred = 32767;
  }
  *index += kIndexTable[nibble];
  if (*index < 0) {
    *index = 0;
  } else if (*index > 88) {
    *index = 88;
  }
  return (int16_t)*pred;
}

struct WavChunks {
  const uint8_t *fmt;
  uint32_t fmtSize;
  const uint8_t *data;
  uint32_t dataSize;
};

static bool findChunks(const uint8_t *wav, size_t len, WavChunks *out) {
  if (len < 44) {
    return false;
  }
  if (std::memcmp(wav, "RIFF", 4) != 0 || std::memcmp(wav + 8, "WAVE", 4) != 0) {
    return false;
  }
  out->fmt = nullptr;
  out->fmtSize = 0;
  out->data = nullptr;
  out->dataSize = 0;

  size_t pos = 12;
  while (pos + 8 <= len) {
    uint32_t chunkSize = rd32(wav + pos + 4);
    if (pos + 8 + chunkSize > len) {
      break;
    }
    if (std::memcmp(wav + pos, "fmt ", 4) == 0) {
      out->fmt = wav + pos + 8;
      out->fmtSize = chunkSize;
    } else if (std::memcmp(wav + pos, "data", 4) == 0) {
      out->data = wav + pos + 8;
      out->dataSize = chunkSize;
    }
    pos += 8 + chunkSize;
    if (pos & 1) {
      pos++;
    }
  }
  return out->fmt && out->fmtSize >= 16 && out->data && out->dataSize > 0;
}

}  // namespace

bool AdpcmWav_DecodeImaToPcm16(
    const uint8_t *wavData,
    size_t wavBytes,
    uint8_t **outPcm,
    uint32_t *outPcmBytes,
    uint32_t *outSampleRate,
    uint16_t *outChannels) {
  if (!wavData || !outPcm || !outPcmBytes || !outSampleRate || !outChannels) {
    return false;
  }
  *outPcm = nullptr;
  *outPcmBytes = 0;

  WavChunks chunks;
  if (!findChunks(wavData, wavBytes, &chunks)) {
    return false;
  }

  const uint16_t format = rd16(chunks.fmt);
  if (format != 17) {
    return false;
  }

  const uint16_t channels = rd16(chunks.fmt + 2);
  const uint32_t sampleRate = rd32(chunks.fmt + 4);
  const uint16_t blockAlign = rd16(chunks.fmt + 12);
  uint16_t samplesPerBlock = 0;
  if (chunks.fmtSize >= 20) {
    samplesPerBlock = rd16(chunks.fmt + 18);
  }

  if (channels < 1 || channels > 2) {
    return false;
  }
  if (sampleRate == 0 || sampleRate > 96000) {
    return false;
  }
  if (blockAlign < (channels == 1 ? 4u : 8u)) {
    return false;
  }
  if (samplesPerBlock == 0) {
    if (channels == 1) {
      samplesPerBlock = (uint16_t)(((blockAlign - 4) * 2) + 1);
    } else {
      samplesPerBlock = (uint16_t)(((blockAlign - 8) * 2) / 2 + 1);
    }
  }

  const uint32_t numBlocks = chunks.dataSize / blockAlign;
  if (numBlocks == 0) {
    return false;
  }

  const uint64_t totalFrames = (uint64_t)numBlocks * (uint64_t)samplesPerBlock;
  const uint64_t totalSamples = totalFrames * (uint64_t)channels;
  const uint64_t outBytes64 = totalSamples * 2ull;
  if (outBytes64 == 0 || outBytes64 > 50ull * 1024ull * 1024ull) {
    return false;
  }

  int16_t *pcm = (int16_t *)std::malloc((size_t)outBytes64);
  if (!pcm) {
    return false;
  }

  uint32_t outIndex = 0;
  const uint8_t *data = chunks.data;

  if (channels == 1) {
    for (uint32_t b = 0; b < numBlocks; ++b) {
      const uint8_t *block = data + (size_t)b * blockAlign;
      int pred = (int16_t)rd16(block);
      int index = block[2];
      pcm[outIndex++] = (int16_t)pred;

      uint16_t framesDone = 1;
      for (uint16_t bi = 4; bi < blockAlign && framesDone < samplesPerBlock; ++bi) {
        const uint8_t byte = block[bi];
        pcm[outIndex++] = decodeNibble(byte & 0x0F, &pred, &index);
        ++framesDone;
        if (framesDone >= samplesPerBlock) {
          break;
        }
        pcm[outIndex++] = decodeNibble((byte >> 4) & 0x0F, &pred, &index);
        ++framesDone;
      }
    }
  } else {
    for (uint32_t b = 0; b < numBlocks; ++b) {
      const uint8_t *block = data + (size_t)b * blockAlign;
      int pred[2] = {(int16_t)rd16(block), (int16_t)rd16(block + 4)};
      int index[2] = {block[2], block[6]};
      pcm[outIndex++] = (int16_t)pred[0];
      pcm[outIndex++] = (int16_t)pred[1];

      uint16_t framesDone = 1;
      uint16_t pos = 8;
      while (framesDone < samplesPerBlock && pos + 8 <= blockAlign) {
        const uint8_t *leftBytes = block + pos;
        const uint8_t *rightBytes = block + pos + 4;
        pos = (uint16_t)(pos + 8);

        int16_t leftS[8];
        int16_t rightS[8];
        int nLeft = 0;
        int nRight = 0;
        for (int i = 0; i < 4; ++i) {
          leftS[nLeft++] = decodeNibble(leftBytes[i] & 0x0F, &pred[0], &index[0]);
          leftS[nLeft++] = decodeNibble((leftBytes[i] >> 4) & 0x0F, &pred[0], &index[0]);
          rightS[nRight++] = decodeNibble(rightBytes[i] & 0x0F, &pred[1], &index[1]);
          rightS[nRight++] = decodeNibble((rightBytes[i] >> 4) & 0x0F, &pred[1], &index[1]);
        }
        for (int i = 0; i < 8 && framesDone < samplesPerBlock; ++i) {
          pcm[outIndex++] = leftS[i];
          pcm[outIndex++] = rightS[i];
          ++framesDone;
        }
      }
    }
  }

  *outPcm = (uint8_t *)pcm;
  *outPcmBytes = outIndex * 2u;
  *outSampleRate = sampleRate;
  *outChannels = channels;
  return outIndex > 0;
}
