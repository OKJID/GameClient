#import "MemDiag.h"
#import <Metal/Metal.h>
#import <objc/runtime.h>

#include <atomic>
#include <cstdio>
#include <mach/mach.h>

// A sentinel is attached to every tracked Metal resource. ARC releases the
// sentinel together with its host, so -dealloc is the exact moment the GPU
// resource went away — that is what makes live counts trustworthy.

namespace {

std::atomic<uint64_t> g_created[MDS_COUNT];
std::atomic<uint64_t> g_freed[MDS_COUNT];
std::atomic<int64_t> g_liveBytes[MDS_COUNT];
uint64_t g_prevCreated[MDS_COUNT];
uint64_t g_prevFreed[MDS_COUNT];

FILE *g_file = nullptr;
bool g_tried = false;

const char *kSiteName[MDS_COUNT] = {
    "VB_CTOR",    "VB_LOCK_DISCARD", "VB_GETMTL",
    "IB_CTOR",    "IB_LOCK_DISCARD", "IB_GETMTL",
    "TEX_CTOR",   "TEX_BACK",        "CUBE_CTOR",
    "CUBE_RESIZE", "RING_FIRST",     "RING_WRAP",
    "UP_OVERSIZE", "FAN_UP",         "FAN_IDX16",
    "FAN_IDX32",  "DEPTH_TEX",       "MSAA_COLOR",
    "MSAA_DEPTH", "ZERO_BUF",        "PSO",
    "SAMPLER",    "DSS",             "CMDBUF_SCENE",
    "CMDBUF_MIPS", "CMDBUF_COPY",    "CMDQUEUE_FILTER",
    "W_VB",       "W_IB",            "W_TEX",
    "W_SURFACE",  "W_CUBE",
};

static_assert(sizeof(kSiteName) / sizeof(kSiteName[0]) == MDS_COUNT,
              "kSiteName must stay in step with MemDiagSite");

uint64_t PhysFootprint() {
  task_vm_info_data_t info;
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&info, &count) !=
      KERN_SUCCESS) {
    return 0;
  }
  return info.phys_footprint;
}

} // namespace

@interface MemDiagSentinel : NSObject
@property(nonatomic) int site;
@property(nonatomic) uint64_t bytes;
@end

@implementation MemDiagSentinel
- (void)dealloc {
  g_freed[_site].fetch_add(1, std::memory_order_relaxed);
  g_liveBytes[_site].fetch_sub((int64_t)_bytes, std::memory_order_relaxed);
}
@end

static const void *kMemDiagKey = &kMemDiagKey;

void MemDiag_Track(id obj, int site, uint64_t bytes) {
  if (!obj || site < 0 || site >= MDS_COUNT) {
    return;
  }

  if ([obj respondsToSelector:@selector(allocatedSize)]) {
    bytes = (uint64_t)[(id<MTLResource>)obj allocatedSize];
  }

  MemDiagSentinel *sentinel = [MemDiagSentinel new];
  sentinel.site = site;
  sentinel.bytes = bytes;
  objc_setAssociatedObject(obj, kMemDiagKey, sentinel,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);

  g_created[site].fetch_add(1, std::memory_order_relaxed);
  g_liveBytes[site].fetch_add((int64_t)bytes, std::memory_order_relaxed);
}

void MemDiag_NoteWrapperCreate(int site) {
  if (site < 0 || site >= MDS_COUNT) {
    return;
  }
  g_created[site].fetch_add(1, std::memory_order_relaxed);
}

void MemDiag_NoteWrapperDestroy(int site) {
  if (site < 0 || site >= MDS_COUNT) {
    return;
  }
  g_freed[site].fetch_add(1, std::memory_order_relaxed);
}

void MemDiag_Frame(int frame, void *mtlDevice) {
  if (frame % 600 != 0) {
    return;
  }

  if (!g_tried) {
    g_tried = true;
    g_file = fopen("MemDiag.txt", "w");
  }

  if (!g_file) {
    return;
  }

  id<MTLDevice> device = (__bridge id<MTLDevice>)mtlDevice;
  unsigned long long metalAlloc =
      device ? (unsigned long long)[device currentAllocatedSize] : 0ull;

  int64_t trackedBytes = 0;
  for (int i = 0; i < MDS_COUNT; ++i) {
    trackedBytes += g_liveBytes[i].load(std::memory_order_relaxed);
  }

  fprintf(g_file,
          "MEM f%d footprint=%llu metalAlloc=%llu tracked=%lld unaccounted=%lld\n",
          frame, (unsigned long long)PhysFootprint(), metalAlloc,
          (long long)trackedBytes,
          (long long)((int64_t)metalAlloc - trackedBytes));

  for (int i = 0; i < MDS_COUNT; ++i) {
    const uint64_t created = g_created[i].load(std::memory_order_relaxed);
    const uint64_t freed = g_freed[i].load(std::memory_order_relaxed);
    if (created == 0) {
      continue;
    }

    fprintf(g_file,
            "SITE f%d name=%s created=%llu freed=%llu live=%llu dNew=%llu "
            "dFree=%llu liveBytes=%lld\n",
            frame, kSiteName[i], (unsigned long long)created,
            (unsigned long long)freed, (unsigned long long)(created - freed),
            (unsigned long long)(created - g_prevCreated[i]),
            (unsigned long long)(freed - g_prevFreed[i]),
            (long long)g_liveBytes[i].load(std::memory_order_relaxed));

    g_prevCreated[i] = created;
    g_prevFreed[i] = freed;
  }

  fflush(g_file);
}
