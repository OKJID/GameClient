#pragma once

// Per-site accounting of Metal resource lifetime. Writes MemDiag.txt.
// Diagnostics only — see .agent/_tasks/memory_leak/02_probe_memdiag.md

#include <cstdint>

enum MemDiagSite {
  MDS_VB_CTOR,
  MDS_VB_LOCK_DISCARD,
  MDS_VB_GETMTL,
  MDS_IB_CTOR,
  MDS_IB_LOCK_DISCARD,
  MDS_IB_GETMTL,
  MDS_TEX_CTOR,
  MDS_TEX_BACK,
  MDS_CUBE_CTOR,
  MDS_CUBE_RESIZE,
  MDS_RING_FIRST,
  MDS_RING_WRAP,
  MDS_UP_OVERSIZE,
  MDS_FAN_UP,
  MDS_FAN_IDX16,
  MDS_FAN_IDX32,
  MDS_DEPTH_TEX,
  MDS_MSAA_COLOR,
  MDS_MSAA_DEPTH,
  MDS_ZERO_BUF,
  MDS_PSO,
  MDS_SAMPLER,
  MDS_DSS,
  MDS_CMDBUF_SCENE,
  MDS_CMDBUF_MIPS,
  MDS_CMDBUF_COPY,
  MDS_CMDQUEUE_FILTER,
  MDS_W_VB,
  MDS_W_IB,
  MDS_W_TEX,
  MDS_W_SURFACE,
  MDS_W_CUBE,
  MDS_COUNT
};

void MemDiag_NoteWrapperCreate(int site);
void MemDiag_NoteWrapperDestroy(int site);
void MemDiag_Frame(int frame, void *mtlDevice);

#ifdef __OBJC__
void MemDiag_Track(id obj, int site, uint64_t bytes);
#define MEMDIAG_TRACK(obj, site, bytes) MemDiag_Track((obj), (site), (uint64_t)(bytes))
#endif
