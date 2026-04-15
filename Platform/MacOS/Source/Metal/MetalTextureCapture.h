/**
 * MetalTextureCapture.h — Texture capture interceptor
 *
 * When enabled via GENERALS_CAPTURE_TEXTURES=1 env var, captures unique
 * textures during gameplay and exports them as a C++ file that can be
 * compiled directly into the test suite.
 *
 * Usage:
 *   GENERALS_CAPTURE_TEXTURES=1 sh build_run_mac.sh --screenshot
 *   # → generates Platform/MacOS/Tests/captured_textures_data.cpp
 *
 * Then rebuild tests and run:
 *   sh build_run_mac.sh --test=captured
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <map>
#include <vector>
#include <string>
#include <d3d8.h>

// ─────────────────────────────────────────────────────
//  Texture Capture System
// ─────────────────────────────────────────────────────

// Forward declare for signal/atexit
static void _TextureCaptureAtExit();
static void _TextureCaptureSignal(int sig);

struct CapturedTextureEntry {
  std::string name;           // e.g. "R5G6B5_64x64_a3f2c1"
  D3DFORMAT format;
  uint32_t width;
  uint32_t height;
  uint32_t srcPitch;
  std::vector<uint8_t> srcData;    // raw source pixels (before conversion)
  uint64_t contentHash;
};

class TextureCaptureSystem {
public:
  static TextureCaptureSystem& Instance() {
    static TextureCaptureSystem s;
    return s;
  }

  bool IsEnabled() const { return m_enabled; }

  void Init() {
    const char* env = getenv("GENERALS_CAPTURE_TEXTURES");
    m_enabled = (env && strcmp(env, "1") == 0);
    if (m_enabled) {
      printf("[TextureCapture] Enabled — will capture unique textures\n");
      fflush(stdout);

      // Register atexit + signal handlers so export happens even on kill
      atexit(_TextureCaptureAtExit);
      signal(SIGTERM, _TextureCaptureSignal);
      signal(SIGINT, _TextureCaptureSignal);
    }
  }

  // Capture device capabilities (call after device is fully initialized)
  void CaptureDeviceCaps(IDirect3DDevice8* device) {
    if (!m_enabled || !device) return;

    D3DDISPLAYMODE mode;
    if (SUCCEEDED(device->GetDisplayMode(&mode))) {
      m_displayWidth = mode.Width;
      m_displayHeight = mode.Height;
      m_displayFormat = (uint32_t)mode.Format;
      // Determine bit depth from format
      if (mode.Format == D3DFMT_A8R8G8B8 || mode.Format == D3DFMT_X8R8G8B8) {
        m_displayBits = 32;
      } else if (mode.Format == D3DFMT_R5G6B5 || mode.Format == D3DFMT_A1R5G5B5 ||
                 mode.Format == D3DFMT_A4R4G4B4 || mode.Format == D3DFMT_X1R5G5B5) {
        m_displayBits = 16;
      } else {
        m_displayBits = 0; // unknown
      }
    }

    D3DCAPS8 caps;
    if (SUCCEEDED(device->GetDeviceCaps(&caps))) {
      m_maxTextureWidth = caps.MaxTextureWidth;
      m_maxTextureHeight = caps.MaxTextureHeight;
    }

    // Check texture format support by attempting CreateTexture with each format.
    // DX8 doesn't have CheckDeviceFormat, so we try creating a tiny texture.
    D3DFORMAT testFormats[] = {
      D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5,
      D3DFMT_A4R4G4B4, D3DFMT_A1R5G5B5, D3DFMT_X1R5G5B5
    };
    for (auto fmt : testFormats) {
      IDirect3DTexture8* testTex = nullptr;
      HRESULT hr = device->CreateTexture(4, 4, 1, 0, fmt, D3DPOOL_MANAGED, &testTex);
      m_formatSupport[(uint32_t)fmt] = SUCCEEDED(hr);
      if (testTex) testTex->Release();
    }

    printf("[TextureCapture] Device caps: %ux%u fmt=%u bits=%u maxTex=%ux%u\n",
           m_displayWidth, m_displayHeight, m_displayFormat, m_displayBits,
           m_maxTextureWidth, m_maxTextureHeight);
    printf("[TextureCapture] Format support: A8R8G8B8=%d A4R4G4B4=%d A1R5G5B5=%d R5G6B5=%d\n",
           (int)m_formatSupport[D3DFMT_A8R8G8B8], (int)m_formatSupport[D3DFMT_A4R4G4B4],
           (int)m_formatSupport[D3DFMT_A1R5G5B5], (int)m_formatSupport[D3DFMT_R5G6B5]);
    fflush(stdout);
  }

  // Called from UnlockRect before conversion
  void CaptureTexture(D3DFORMAT format, uint32_t width, uint32_t height,
                      uint32_t pitch, const void* srcData, uint32_t dataSize) {
    if (!m_enabled || !srcData || dataSize == 0) return;

    // Compute content hash for deduplication
    uint64_t hash = FNV1a(srcData, dataSize);
    hash ^= ((uint64_t)format << 32) | ((uint64_t)width << 16) | height;

    // Skip duplicates
    if (m_seen.count(hash)) return;
    m_seen[hash] = true;

    CapturedTextureEntry entry;
    char nameBuf[128];
    snprintf(nameBuf, sizeof(nameBuf), "fmt%u_%ux%u_%llx",
             (unsigned)format, width, height, (unsigned long long)hash);
    entry.name = nameBuf;
    entry.format = format;
    entry.width = width;
    entry.height = height;
    entry.srcPitch = pitch;
    entry.srcData.assign((const uint8_t*)srcData,
                          (const uint8_t*)srcData + dataSize);
    entry.contentHash = hash;

    m_captures.push_back(std::move(entry));

    printf("[TextureCapture] Captured #%zu: %s (fmt=%u, %ux%u, %u bytes)\n",
           m_captures.size(), nameBuf, (unsigned)format, width, height, dataSize);
    fflush(stdout);
  }

  // Export all captured textures as a C++ source file
  void ExportCpp(const char* outputPath) {
    if (m_exported || m_captures.empty()) {
      if (!m_exported && m_captures.empty()) {
        printf("[TextureCapture] No textures captured, skipping export\n");
      }
      return;
    }
    m_exported = true;

    FILE* f = fopen(outputPath, "w");
    if (!f) {
      printf("[TextureCapture] ERROR: Cannot open %s for writing\n", outputPath);
      return;
    }

    fprintf(f, "/**\n * Auto-generated texture test data\n");
    fprintf(f, " * Captured %zu unique textures from gameplay\n", m_captures.size());
    fprintf(f, " *\n * To regenerate:\n");
    fprintf(f, " *   GENERALS_CAPTURE_TEXTURES=1 sh build_run_mac.sh --screenshot\n");
    fprintf(f, " */\n\n");
    fprintf(f, "// This file is #included into test_captured_textures.cpp\n\n");

    // ── Device Capabilities ──
    fprintf(f, "#define HAS_CAPTURED_DEVICE_CAPS 1\n");
    fprintf(f, "// ═══ Device Capabilities at capture time ═══\n");
    fprintf(f, "static const uint32_t captured_display_width = %u;\n", m_displayWidth);
    fprintf(f, "static const uint32_t captured_display_height = %u;\n", m_displayHeight);
    fprintf(f, "static const uint32_t captured_display_format = %u;\n", m_displayFormat);
    fprintf(f, "static const uint32_t captured_display_bits = %u;\n", m_displayBits);
    fprintf(f, "static const uint32_t captured_max_tex_width = %u;\n", m_maxTextureWidth);
    fprintf(f, "static const uint32_t captured_max_tex_height = %u;\n", m_maxTextureHeight);
    fprintf(f, "\n// Texture format support (true = CreateTexture succeeded)\n");
    fprintf(f, "static const bool captured_support_A8R8G8B8 = %s;\n",
            m_formatSupport.count(D3DFMT_A8R8G8B8) && m_formatSupport[D3DFMT_A8R8G8B8] ? "true" : "false");
    fprintf(f, "static const bool captured_support_X8R8G8B8 = %s;\n",
            m_formatSupport.count(D3DFMT_X8R8G8B8) && m_formatSupport[D3DFMT_X8R8G8B8] ? "true" : "false");
    fprintf(f, "static const bool captured_support_R5G6B5 = %s;\n",
            m_formatSupport.count(D3DFMT_R5G6B5) && m_formatSupport[D3DFMT_R5G6B5] ? "true" : "false");
    fprintf(f, "static const bool captured_support_A4R4G4B4 = %s;\n",
            m_formatSupport.count(D3DFMT_A4R4G4B4) && m_formatSupport[D3DFMT_A4R4G4B4] ? "true" : "false");
    fprintf(f, "static const bool captured_support_A1R5G5B5 = %s;\n",
            m_formatSupport.count(D3DFMT_A1R5G5B5) && m_formatSupport[D3DFMT_A1R5G5B5] ? "true" : "false");
    fprintf(f, "static const bool captured_support_X1R5G5B5 = %s;\n\n",
            m_formatSupport.count(D3DFMT_X1R5G5B5) && m_formatSupport[D3DFMT_X1R5G5B5] ? "true" : "false");

    // ── Texture struct ──
    fprintf(f, "struct CapturedTexture {\n");
    fprintf(f, "  const char* name;\n");
    fprintf(f, "  D3DFORMAT format;\n");
    fprintf(f, "  uint32_t width, height, srcPitch;\n");
    fprintf(f, "  const uint8_t* srcData;\n");
    fprintf(f, "  uint32_t srcSize;\n");
    fprintf(f, "};\n\n");

    // Emit byte arrays
    for (size_t i = 0; i < m_captures.size(); i++) {
      const auto& cap = m_captures[i];
      fprintf(f, "static const uint8_t tex_%zu_src[] = {\n  ", i);
      for (size_t j = 0; j < cap.srcData.size(); j++) {
        fprintf(f, "0x%02X", cap.srcData[j]);
        if (j + 1 < cap.srcData.size()) {
          fprintf(f, ",");
          if ((j + 1) % 16 == 0) fprintf(f, "\n  ");
        }
      }
      fprintf(f, "\n};\n\n");
    }

    // Emit texture table
    fprintf(f, "static const CapturedTexture captured_textures[] = {\n");
    for (size_t i = 0; i < m_captures.size(); i++) {
      const auto& cap = m_captures[i];
      fprintf(f, "  {\"%s\", (D3DFORMAT)%u, %u, %u, %u, tex_%zu_src, %zu},\n",
              cap.name.c_str(), (unsigned)cap.format,
              cap.width, cap.height, cap.srcPitch,
              i, cap.srcData.size());
    }
    fprintf(f, "};\n\n");
    fprintf(f, "static const size_t captured_texture_count = %zu;\n", m_captures.size());

    fclose(f);
    printf("[TextureCapture] Exported %zu textures to %s\n",
           m_captures.size(), outputPath);
    fflush(stdout);
  }

private:
  TextureCaptureSystem() : m_enabled(false), m_exported(false) {}

  static uint64_t FNV1a(const void* data, size_t len) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    const uint8_t* p = (const uint8_t*)data;
    for (size_t i = 0; i < len; i++) {
      hash ^= p[i];
      hash *= 0x100000001b3ULL;
    }
    return hash;
  }

  bool m_enabled;
  bool m_exported = false;
  std::map<uint64_t, bool> m_seen;
  std::vector<CapturedTextureEntry> m_captures;

  // Device capabilities (captured at init)
  uint32_t m_displayWidth = 0, m_displayHeight = 0;
  uint32_t m_displayFormat = 0, m_displayBits = 0;
  uint32_t m_maxTextureWidth = 0, m_maxTextureHeight = 0;
  std::map<uint32_t, bool> m_formatSupport;
};

// ── Signal/atexit handlers ──

static void _TextureCaptureAtExit() {
  auto& sys = TextureCaptureSystem::Instance();
  if (sys.IsEnabled()) {
    sys.ExportCpp("Platform/MacOS/Tests/captured_textures_data.cpp");
  }
}

static void _TextureCaptureSignal(int sig) {
  _TextureCaptureAtExit();
  // Re-raise signal with default handler for clean exit
  signal(sig, SIG_DFL);
  raise(sig);
}
