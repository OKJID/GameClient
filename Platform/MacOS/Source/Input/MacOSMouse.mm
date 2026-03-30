#include "MacOSMouse.h"
#include "Common/GlobalData.h"
#include "Common/File.h"
#include "Common/FileSystem.h"
#include "GameClient/Display.h"
#include "GameClient/GameWindow.h"
#include "GameClient/Image.h"
#include "GameClient/InGameUI.h"
#include "always.h"
#include "W3DDevice/GameClient/W3DAssetManager.h"
#include "W3DDevice/GameClient/W3DDisplay.h"
#include "W3DDevice/GameClient/W3DScene.h"
#include "W3DDevice/Common/W3DConvert.h"
#include "WW3D2/render2d.h"
#include "WW3D2/texture.h"
#include "WW3D2/hanim.h"
#include "WW3D2/camera.h"
#include "WW3D2/ww3d.h"
#include "WW3D2/rendobj.h"
#import <AppKit/AppKit.h>



MacOSMouse::MacOSMouse(void) {
  m_nextFreeIndex = 0;
  m_nextGetIndex = 0;
  m_w3dCamera = nullptr;
  m_currentW3DCursor = NONE;
  m_w3dAssetsLoaded = false;
  m_aniCursorsLoaded = false;
  m_directionFrame = 0;
  for (int i = 0; i < NUM_MOUSE_CURSORS; i++) {
    m_cursorModels[i] = nullptr;
    m_cursorAnims[i] = nullptr;
    m_nsCursorFrameCount[i] = 0;
    m_nsCursorCycleMs[i] = 0;
    for (int j = 0; j < MAX_2D_CURSOR_ANIM_FRAMES; j++) {
      m_nsCursorStepRateMs[i][j] = 166;
    }
    for (int j = 0; j < MAX_2D_CURSOR_ANIM_FRAMES; j++) {
      m_nsCursors[i][j] = nil;
    }
  }

}

MacOSMouse::~MacOSMouse(void) {

  freeW3DAssets();
  freeANICursors();
}

void MacOSMouse::init(void) {

  Mouse::init();
  m_inputMovesAbsolute = TRUE;
  setVisibility(TRUE);

}

void MacOSMouse::reset(void) {

  Mouse::reset();
  m_inputMovesAbsolute = TRUE;
  m_nextFreeIndex = 0;
  m_nextGetIndex = 0;
}

void MacOSMouse::update(void) { Mouse::update(); }

void MacOSMouse::initCursorResources(void) {

  loadANICursors();
}

NSCursor *MacOSMouse::parseCURData(uint8_t *iconData, int iconSize, int *outHotX, int *outHotY) {
  @autoreleasepool {
    if (iconSize < 22) return nil;

    uint16_t imgType = *(uint16_t *)(iconData + 2);
    uint16_t imgCount = *(uint16_t *)(iconData + 4);
    if (imgCount < 1) return nil;

    int width = iconData[6];
    int height = iconData[7];
    if (width == 0) width = 256;
    if (height == 0) height = 256;

    int hotX = 0, hotY = 0;
    if (imgType == 2) {
      hotX = *(uint16_t *)(iconData + 10);
      hotY = *(uint16_t *)(iconData + 12);
    }
    if (outHotX) *outHotX = hotX;
    if (outHotY) *outHotY = hotY;

    uint32_t imgDataOffset = *(uint32_t *)(iconData + 18);
    if (imgDataOffset + 40 > (uint32_t)iconSize) return nil;

    uint8_t *bmpData = iconData + imgDataOffset;
    int bmpWidth = *(int32_t *)(bmpData + 4);
    int bmpHeight = *(int32_t *)(bmpData + 8);
    int bmpBits = *(uint16_t *)(bmpData + 14);
    int bmpHeaderSize = *(int32_t *)(bmpData + 0);

    if (bmpHeight > 0) bmpHeight /= 2;
    if (bmpWidth <= 0 || bmpHeight <= 0) return nil;

    int w = bmpWidth;
    int h = bmpHeight;

    uint8_t *rgba = (uint8_t *)calloc(w * h * 4, 1);
    uint8_t *palette = bmpData + bmpHeaderSize;
    int paletteCount = (bmpBits <= 8) ? (1 << bmpBits) : 0;
    uint8_t *pixelData = bmpData + bmpHeaderSize + paletteCount * 4;

    if (bmpBits == 4) {
      int rowBytes = ((w + 1) / 2 + 3) & ~3;
      int andRowBytes = ((w + 31) / 32) * 4;
      uint8_t *andMask = pixelData + rowBytes * h;
      for (int y = 0; y < h; y++) {
        uint8_t *src = pixelData + (h - 1 - y) * rowBytes;
        uint8_t *mask = andMask + (h - 1 - y) * andRowBytes;
        uint8_t *dst = rgba + y * w * 4;
        for (int x = 0; x < w; x++) {
          int idx = (x % 2 == 0) ? (src[x / 2] >> 4) : (src[x / 2] & 0x0F);
          dst[x * 4 + 0] = palette[idx * 4 + 2];
          dst[x * 4 + 1] = palette[idx * 4 + 1];
          dst[x * 4 + 2] = palette[idx * 4 + 0];
          int bit = (mask[x / 8] >> (7 - (x % 8))) & 1;
          dst[x * 4 + 3] = bit ? 0 : 255;
        }
      }
    } else if (bmpBits == 8) {
      int rowBytes = (w + 3) & ~3;
      int andRowBytes = ((w + 31) / 32) * 4;
      uint8_t *andMask = pixelData + rowBytes * h;
      for (int y = 0; y < h; y++) {
        uint8_t *src = pixelData + (h - 1 - y) * rowBytes;
        uint8_t *mask = andMask + (h - 1 - y) * andRowBytes;
        uint8_t *dst = rgba + y * w * 4;
        for (int x = 0; x < w; x++) {
          int idx = src[x];
          dst[x * 4 + 0] = palette[idx * 4 + 2];
          dst[x * 4 + 1] = palette[idx * 4 + 1];
          dst[x * 4 + 2] = palette[idx * 4 + 0];
          int bit = (mask[x / 8] >> (7 - (x % 8))) & 1;
          dst[x * 4 + 3] = bit ? 0 : 255;
        }
      }
    } else if (bmpBits == 24) {
      int rowBytes = ((w * 3 + 3) / 4) * 4;
      int andRowBytes = ((w + 31) / 32) * 4;
      uint8_t *andMask = pixelData + rowBytes * h;
      for (int y = 0; y < h; y++) {
        uint8_t *src = pixelData + (h - 1 - y) * rowBytes;
        uint8_t *mask = andMask + (h - 1 - y) * andRowBytes;
        uint8_t *dst = rgba + y * w * 4;
        for (int x = 0; x < w; x++) {
          dst[x * 4 + 0] = src[x * 3 + 2];
          dst[x * 4 + 1] = src[x * 3 + 1];
          dst[x * 4 + 2] = src[x * 3 + 0];
          int bit = (mask[x / 8] >> (7 - (x % 8))) & 1;
          dst[x * 4 + 3] = bit ? 0 : 255;
        }
      }
    } else if (bmpBits == 32) {
      int rowBytes = w * 4;
      for (int y = 0; y < h; y++) {
        uint8_t *src = pixelData + (h - 1 - y) * rowBytes;
        uint8_t *dst = rgba + y * w * 4;
        for (int x = 0; x < w; x++) {
          dst[x * 4 + 0] = src[x * 4 + 2];
          dst[x * 4 + 1] = src[x * 4 + 1];
          dst[x * 4 + 2] = src[x * 4 + 0];
          dst[x * 4 + 3] = src[x * 4 + 3];
        }
      }
    }

    NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:NULL
      pixelsWide:w pixelsHigh:h
      bitsPerSample:8 samplesPerPixel:4
      hasAlpha:YES isPlanar:NO
      colorSpaceName:NSDeviceRGBColorSpace
      bytesPerRow:w * 4 bitsPerPixel:32];
    memcpy([rep bitmapData], rgba, w * h * 4);

    NSImage *nsImage = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
    [nsImage addRepresentation:rep];

    NSCursor *cursor = [[NSCursor alloc] initWithImage:nsImage hotSpot:NSMakePoint(hotX, hotY)];
    free(rgba);
    return cursor;
  }
}

int MacOSMouse::loadANIFrames(const char *path, NSCursor * __strong outCursors[], int maxFrames, int *outStepRatesMs, int *outCycleTotalMs) {
  File *file = TheFileSystem->openFile(path);
  if (!file) return 0;

  long fileSize = file->size();
  if (fileSize < 20) { file->close(); return 0; }

  uint8_t *data = (uint8_t *)malloc(fileSize);
  file->read(data, fileSize);
  file->close();

  if (memcmp(data, "RIFF", 4) != 0 || memcmp(data + 8, "ACON", 4) != 0) {
    free(data);
    return 0;
  }

  int numSteps = 0;
  int dispRate = 10; // default jiffies
  int iconOffsets[MAX_2D_CURSOR_ANIM_FRAMES] = {};
  int iconSizes[MAX_2D_CURSOR_ANIM_FRAMES] = {};
  int iconCount = 0;
  int seqData[MAX_2D_CURSOR_ANIM_FRAMES] = {};
  bool hasSeq = false;
  int perStepRates[MAX_2D_CURSOR_ANIM_FRAMES] = {};
  bool hasRateChunk = false;
  int rateChunkCount = 0;

  long pos = 12;
  while (pos + 8 <= fileSize) {
    uint32_t chunkId = *(uint32_t *)(data + pos);
    uint32_t chunkSize = *(uint32_t *)(data + pos + 4);

    if (chunkId == 0x68696E61) { // 'anih'
      if (pos + 8 + 36 <= fileSize) {
        numSteps = *(int32_t *)(data + pos + 8 + 8);
        dispRate = *(int32_t *)(data + pos + 8 + 28);
      }
    } else if (chunkId == 0x65746172) { // 'rate'
      rateChunkCount = chunkSize / 4;
      if (rateChunkCount > MAX_2D_CURSOR_ANIM_FRAMES) rateChunkCount = MAX_2D_CURSOR_ANIM_FRAMES;
      for (int i = 0; i < rateChunkCount; i++) {
        perStepRates[i] = *(int32_t *)(data + pos + 8 + i * 4);
      }
      hasRateChunk = true;
    } else if (chunkId == 0x20716573) { // 'seq '
      int seqCount = chunkSize / 4;
      if (seqCount > MAX_2D_CURSOR_ANIM_FRAMES) seqCount = MAX_2D_CURSOR_ANIM_FRAMES;
      for (int i = 0; i < seqCount; i++) {
        seqData[i] = *(int32_t *)(data + pos + 8 + i * 4);
      }
      hasSeq = true;
    } else if (chunkId == 0x5453494C) { // 'LIST'
      uint32_t listType = *(uint32_t *)(data + pos + 8);
      if (listType == 0x6D617266) { // 'fram'
        long fpos = pos + 12;
        long listEnd = pos + 8 + chunkSize;
        while (fpos + 8 <= listEnd && iconCount < MAX_2D_CURSOR_ANIM_FRAMES) {
          uint32_t fId = *(uint32_t *)(data + fpos);
          uint32_t fSize = *(uint32_t *)(data + fpos + 4);
          if (fId == 0x6E6F6369) { // 'icon'
            iconOffsets[iconCount] = (int)(fpos + 8);
            iconSizes[iconCount] = (int)fSize;
            iconCount++;
          }
          fpos += 8 + fSize;
          if (fSize % 2) fpos++;
        }
      }
      pos += 8 + chunkSize;
      if (chunkSize % 2) pos++;
      continue;
    }

    pos += 8 + chunkSize;
    if (chunkSize % 2) pos++;
  }



  // Parse icon frames into NSCursors
  NSCursor *iconCursors[MAX_2D_CURSOR_ANIM_FRAMES] = {};
  for (int i = 0; i < iconCount && i < MAX_2D_CURSOR_ANIM_FRAMES; i++) {
    int hotX, hotY;
    iconCursors[i] = parseCURData(data + iconOffsets[i], iconSizes[i], &hotX, &hotY);
  }

  // Build output array using seq if present, and per-step rates
  int outputCount = 0;
  int cycleTotalMs = 0;
  if (hasSeq && numSteps > 0) {
    for (int i = 0; i < numSteps && outputCount < maxFrames; i++) {
      int idx = seqData[i];
      if (idx >= 0 && idx < iconCount && iconCursors[idx]) {
        outCursors[outputCount] = iconCursors[idx];
        int jiffies = (hasRateChunk && i < rateChunkCount) ? perStepRates[i] : dispRate;
        int ms = jiffies * 1000 / 60;
        if (ms < 16) ms = 16;
        if (outStepRatesMs) outStepRatesMs[outputCount] = ms;
        cycleTotalMs += ms;
        outputCount++;
      }
    }
  } else {
    for (int i = 0; i < iconCount && outputCount < maxFrames; i++) {
      if (iconCursors[i]) {
        outCursors[outputCount] = iconCursors[i];
        int jiffies = (hasRateChunk && i < rateChunkCount) ? perStepRates[i] : dispRate;
        int ms = jiffies * 1000 / 60;
        if (ms < 16) ms = 16;
        if (outStepRatesMs) outStepRatesMs[outputCount] = ms;
        cycleTotalMs += ms;
        outputCount++;
      }
    }
  }

  if (outCycleTotalMs) *outCycleTotalMs = cycleTotalMs;
  free(data);
  return outputCount;
}

void MacOSMouse::loadANICursors() {
  if (m_aniCursorsLoaded) return;


  int totalLoaded = 0;

  for (int cursor = FIRST_CURSOR; cursor < NUM_MOUSE_CURSORS; cursor++) {
    if (m_cursorInfo[cursor].textureName.isEmpty()) continue;

    int numDirs = m_cursorInfo[cursor].numDirections;
    if (numDirs < 1) numDirs = 1;

    int loaded = 0;
    int stepRates[MAX_2D_CURSOR_ANIM_FRAMES] = {};
    int cycleMs = 0;

    if (numDirs > 1) {
      for (int dir = 0; dir < numDirs && dir < MAX_2D_CURSOR_ANIM_FRAMES; dir++) {
        char resourcePath[256];
        snprintf(resourcePath, sizeof(resourcePath), "data/cursors/%s%d.ANI",
          m_cursorInfo[cursor].textureName.str(), dir);

        NSCursor *frames[1] = {};
        int count = loadANIFrames(resourcePath, frames, 1, stepRates, &cycleMs);
        if (count > 0) {
          m_nsCursors[cursor][dir] = frames[0];
          loaded++;
        }
      }
    } else {
      char resourcePath[256];
      snprintf(resourcePath, sizeof(resourcePath), "data/cursors/%s.ANI",
        m_cursorInfo[cursor].textureName.str());

      NSCursor *frames[MAX_2D_CURSOR_ANIM_FRAMES] = {};
      loaded = loadANIFrames(resourcePath, frames, MAX_2D_CURSOR_ANIM_FRAMES, stepRates, &cycleMs);
      for (int i = 0; i < loaded; i++) {
        m_nsCursors[cursor][i] = frames[i];
        m_nsCursorStepRateMs[cursor][i] = stepRates[i];
      }
    }

    m_nsCursorFrameCount[cursor] = loaded;
    m_nsCursorCycleMs[cursor] = cycleMs;
    totalLoaded += loaded;

    if (loaded > 0) {
    }
  }


  m_aniCursorsLoaded = true;
}

void MacOSMouse::freeANICursors() {
  @autoreleasepool {
    for (int i = 0; i < NUM_MOUSE_CURSORS; i++) {
      for (int j = 0; j < MAX_2D_CURSOR_ANIM_FRAMES; j++) {
        m_nsCursors[i][j] = nil;
      }
      m_nsCursorFrameCount[i] = 0;
    }
    m_aniCursorsLoaded = false;
  }
}

void MacOSMouse::setCursorDirection(MouseCursor cursor) {
  if (m_cursorInfo[cursor].numDirections > 1 && TheInGameUI && TheInGameUI->isScrolling()) {
    Coord2D offset = TheInGameUI->getScrollAmount();
    if (offset.x != 0 || offset.y != 0) {
      offset.normalize();
      Real theta = atan2(offset.y, offset.x);
      theta = fmod(theta + M_PI * 2, M_PI * 2);
      int numDirs = m_cursorInfo[cursor].numDirections;
      m_directionFrame = (int)(theta / (2.0f * M_PI / (Real)numDirs) + 0.5f);
      if (m_directionFrame >= numDirs) m_directionFrame = 0;
    } else {
      m_directionFrame = 0;
    }
  } else {
    m_directionFrame = 0;
  }
}

void MacOSMouse::initW3DAssets() {
  if (m_w3dAssetsLoaded || !W3DDisplay::m_assetManager) {

    return;
  }



  int modelsLoaded = 0;
  for (int i = 1; i < NUM_MOUSE_CURSORS; i++) {
    if (!m_cursorInfo[i].W3DModelName.isEmpty()) {
      if (m_orthoCamera) {
        m_cursorModels[i] = W3DDisplay::m_assetManager->Create_Render_Obj(m_cursorInfo[i].W3DModelName.str(), m_cursorInfo[i].W3DScale * m_orthoZoom, 0);
      } else {
        m_cursorModels[i] = W3DDisplay::m_assetManager->Create_Render_Obj(m_cursorInfo[i].W3DModelName.str(), m_cursorInfo[i].W3DScale, 0);
      }
      if (m_cursorModels[i]) {
        m_cursorModels[i]->Set_Position(Vector3(0.0f, 0.0f, -1.0f));
        modelsLoaded++;
      }
    }
  }

  int animsLoaded = 0;
  for (int i = 1; i < NUM_MOUSE_CURSORS; i++) {
    if (!m_cursorInfo[i].W3DAnimName.isEmpty()) {
      m_cursorAnims[i] = W3DDisplay::m_assetManager->Get_HAnim(m_cursorInfo[i].W3DAnimName.str());
      if (m_cursorAnims[i] && m_cursorModels[i]) {
        m_cursorModels[i]->Set_Animation(m_cursorAnims[i], 0, (m_cursorInfo[i].loop) ? RenderObjClass::ANIM_MODE_LOOP : RenderObjClass::ANIM_MODE_ONCE);
        animsLoaded++;
      }
    }
  }

  m_w3dCamera = new CameraClass();
  m_w3dCamera->Set_Position(Vector3(0, 1, 1));
  Vector2 min = Vector2(-1, -1);
  Vector2 max = Vector2(+1, +1);
  m_w3dCamera->Set_View_Plane(min, max);
  m_w3dCamera->Set_Clip_Planes(0.995f, 20.0f);
  if (m_orthoCamera) {
    m_w3dCamera->Set_Projection_Type(CameraClass::ORTHO);
  }

  m_w3dAssetsLoaded = true;

}

void MacOSMouse::freeW3DAssets() {

  for (int i = 0; i < NUM_MOUSE_CURSORS; i++) {
    if (W3DDisplay::m_3DInterfaceScene && m_cursorModels[i]) {
      W3DDisplay::m_3DInterfaceScene->Remove_Render_Object(m_cursorModels[i]);
    }
    REF_PTR_RELEASE(m_cursorModels[i]);
    REF_PTR_RELEASE(m_cursorAnims[i]);
  }
  REF_PTR_RELEASE(m_w3dCamera);
  m_w3dAssetsLoaded = false;
  m_currentW3DCursor = NONE;
}

void MacOSMouse::setRedrawMode(RedrawMode mode) {

  MouseCursor cursor = getMouseCursor();
  setCursor(NONE);
  m_currentRedrawMode = mode;
  if (mode == RM_W3D) {
    initW3DAssets();
  } else {
    freeW3DAssets();
  }
  setCursor(cursor);
}

void MacOSMouse::setCursor(MouseCursor cursor) {

  Mouse::setCursor(cursor);

  setCursorDirection(cursor);

  if (m_currentRedrawMode == RM_WINDOWS) {
    @autoreleasepool {
      if (cursor == NONE || !m_visible) {
        [NSCursor unhide];
        [[NSCursor arrowCursor] set];
      } else if (m_cursorInfo[cursor].numDirections > 1) {
        int frame = m_directionFrame;
        if (frame >= m_nsCursorFrameCount[cursor]) frame = 0;
        NSCursor *nsCur = m_nsCursors[cursor][frame];
        if (nsCur) {
          [nsCur set];
        }
      } else if (m_nsCursorFrameCount[cursor] <= 1) {
        NSCursor *nsCur = m_nsCursors[cursor][0];
        if (nsCur) {
          [nsCur set];
        }
      }
    }
  }

  if (m_currentRedrawMode == RM_W3D) {
    if (cursor != m_currentW3DCursor) {
      if (!m_w3dAssetsLoaded) {
        initW3DAssets();
      }
      if (m_currentW3DCursor != NONE && m_cursorModels[m_currentW3DCursor] && W3DDisplay::m_3DInterfaceScene) {
        W3DDisplay::m_3DInterfaceScene->Remove_Render_Object(m_cursorModels[m_currentW3DCursor]);
      }
      m_currentW3DCursor = cursor;
      if (m_currentW3DCursor != NONE && m_cursorModels[m_currentW3DCursor] && W3DDisplay::m_3DInterfaceScene) {
        W3DDisplay::m_3DInterfaceScene->Add_Render_Object(m_cursorModels[m_currentW3DCursor]);
        if (m_cursorInfo[m_currentW3DCursor].loop == FALSE && m_cursorAnims[m_currentW3DCursor]) {
          m_cursorModels[m_currentW3DCursor]->Set_Animation(m_cursorAnims[m_currentW3DCursor], 0, RenderObjClass::ANIM_MODE_ONCE);
        }
      }
    } else {
      m_currentW3DCursor = cursor;
    }
  }

  m_currentCursor = cursor;
}

void MacOSMouse::setVisibility(Bool visible) {

  m_visible = visible;
}

void MacOSMouse::draw(void) {
  setCursor(m_currentCursor);

  if (m_currentRedrawMode == RM_WINDOWS) {
    @autoreleasepool {
      if (m_currentCursor != NONE && m_visible) {
        int frameCount = m_nsCursorFrameCount[m_currentCursor];
        int frame = 0;

        if (m_cursorInfo[m_currentCursor].numDirections > 1) {
          frame = m_directionFrame;
        } else if (frameCount > 1) {
          static unsigned int animStartTime = 0;
          static int lastCursorForAnim = -1;
          unsigned int now = timeGetTime();
          if (lastCursorForAnim != m_currentCursor || animStartTime == 0) {
            animStartTime = now;
            lastCursorForAnim = m_currentCursor;
          }
          int cycleMs = m_nsCursorCycleMs[m_currentCursor];
          if (cycleMs <= 0) cycleMs = frameCount * 166;
          unsigned int elapsed = (now - animStartTime) % cycleMs;
          int accum = 0;
          for (int i = 0; i < frameCount; i++) {
            accum += m_nsCursorStepRateMs[m_currentCursor][i];
            if ((int)elapsed < accum) { frame = i; break; }
          }
        }

        if (frame >= frameCount) frame = 0;
        NSCursor *nsCur = m_nsCursors[m_currentCursor][frame];
        if (nsCur) {
          [nsCur set];
        }
      }
    }
    drawCursorText();
    if (m_visible) drawTooltip();
    return;
  }

  if (m_currentRedrawMode == RM_W3D) {
    if (W3DDisplay::m_3DInterfaceScene && m_w3dCamera && m_visible) {
      if (m_currentW3DCursor != NONE && m_cursorModels[m_currentW3DCursor]) {
        Real xPercent = (1.0f - (TheDisplay->getWidth() - m_currMouse.pos.x) / (Real)TheDisplay->getWidth());
        Real yPercent = ((TheDisplay->getHeight() - m_currMouse.pos.y) / (Real)TheDisplay->getHeight());

        Real x, y, z = -1.0f;

        if (m_orthoCamera) {
          x = xPercent * 2 - 1;
          y = yPercent * 2;
        } else {
          Real logX, logY;
          PixelScreenToW3DLogicalScreen(m_currMouse.pos.x, m_currMouse.pos.y, &logX, &logY, TheDisplay->getWidth(), TheDisplay->getHeight());

          Vector3 rayStart;
          Vector3 rayEnd;
          rayStart = m_w3dCamera->Get_Position();
          m_w3dCamera->Un_Project(rayEnd, Vector2(logX, logY));
          rayEnd -= rayStart;
          rayEnd.Normalize();
          rayEnd *= m_w3dCamera->Get_Depth();
          rayEnd += rayStart;

          x = Vector3::Find_X_At_Z(z, rayStart, rayEnd);
          y = Vector3::Find_Y_At_Z(z, rayStart, rayEnd);
        }

        Matrix3D tm(1);
        tm.Set_Translation(Vector3(x, y, z));
        Coord2D offset = {0, 0};
        if (TheInGameUI && TheInGameUI->isScrolling()) {
          offset = TheInGameUI->getScrollAmount();
          offset.normalize();
          Real theta = atan2(-offset.y, offset.x);
          theta -= (Real)M_PI / 2;
          tm.Rotate_Z(theta);
        }
        m_cursorModels[m_currentW3DCursor]->Set_Transform(tm);

        WW3D::Render(W3DDisplay::m_3DInterfaceScene, m_w3dCamera);
      }
    }
    drawCursorText();
    if (m_visible) drawTooltip();
    return;
  }

  // Fallback: green crosshair
  if (TheDisplay) {
    int cx = m_currMouse.pos.x;
    int cy = m_currMouse.pos.y;
    TheDisplay->drawFillRect(cx - 8, cy - 1, 16, 2, GameMakeColor(0, 255, 0, 200));
    TheDisplay->drawFillRect(cx - 1, cy - 8, 2, 16, GameMakeColor(0, 255, 0, 200));
  }
  drawCursorText();
  drawTooltip();
}

void MacOSMouse::capture(void) {

  onCursorCaptured(TRUE);
}

void MacOSMouse::releaseCapture(void) {

  onCursorCaptured(FALSE);
}

void MacOSMouse::regainFocus() {

  Mouse::regainFocus();
}

void MacOSMouse::loseFocus() {

  Mouse::loseFocus();
}

UnsignedByte MacOSMouse::getMouseEvent(MouseIO *result, Bool flush) {
  if (m_nextGetIndex == m_nextFreeIndex) {
    return MOUSE_NONE;
  }

  MacOSMouseEvent &ev = m_eventBuffer[m_nextGetIndex];
  m_nextGetIndex = (m_nextGetIndex + 1) % MAX_EVENTS;

  result->leftState = result->middleState = result->rightState = MBS_None;
  result->pos.x = result->pos.y = result->wheelPos = 0;
  result->time = ev.time;

  result->pos.x = ev.x;
  result->pos.y = ev.y;

  switch (ev.type) {
  case MACOS_MOUSE_LBUTTON_DOWN:
    result->leftState = MBS_Down;
    break;
  case MACOS_MOUSE_LBUTTON_UP:
    result->leftState = MBS_Up;
    break;
  case MACOS_MOUSE_LBUTTON_DBLCLK:
    result->leftState = MBS_DoubleClick;
    break;
  case MACOS_MOUSE_RBUTTON_DOWN:
    result->rightState = MBS_Down;
    break;
  case MACOS_MOUSE_RBUTTON_UP:
    result->rightState = MBS_Up;
    break;
  case MACOS_MOUSE_RBUTTON_DBLCLK:
    result->rightState = MBS_DoubleClick;
    break;
  case MACOS_MOUSE_MBUTTON_DOWN:
    result->middleState = MBS_Down;
    break;
  case MACOS_MOUSE_MBUTTON_UP:
    result->middleState = MBS_Up;
    break;
  case MACOS_MOUSE_WHEEL:
    result->wheelPos = ev.wheelDelta;
    break;
  default:
    break;
  }

  return MOUSE_OK;
}

void MacOSMouse::addEvent(int type, int x, int y, int button, int wheelDelta,
                        unsigned int time) {
  static int lastX = -1, lastY = -1;
  if (type == MACOS_MOUSE_MOVE && x == lastX && y == lastY) {
    return;
  }
  if (type == MACOS_MOUSE_MOVE) {
    lastX = x;
    lastY = y;
  }

  unsigned int nextIndex = (m_nextFreeIndex + 1) % MAX_EVENTS;
  if (nextIndex == m_nextGetIndex) {
    m_nextGetIndex = (m_nextGetIndex + 1) % MAX_EVENTS;
  }

  MacOSMouseEvent &ev = m_eventBuffer[m_nextFreeIndex];
  ev.type = type;
  ev.x = x;
  ev.y = y;
  ev.button = button;
  ev.wheelDelta = wheelDelta;
  ev.time = time;

  m_nextFreeIndex = nextIndex;
}
