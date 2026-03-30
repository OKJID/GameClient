/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
*/

#pragma once

#include "GameClient/Mouse.h"

#ifdef __OBJC__
@class NSCursor;
#else
typedef void NSCursor;
#endif

class CameraClass;
class RenderObjClass;
class HAnimClass;

class MacOSMouse : public Mouse {
public:
  MacOSMouse(void);
  virtual ~MacOSMouse(void);

  virtual void init(void) override;
  virtual void reset(void) override;
  virtual void update(void) override;
  virtual void initCursorResources(void) override;

  virtual void setCursor(MouseCursor cursor) override;
  virtual void setVisibility(Bool visible) override;
  virtual void draw(void) override;
  virtual void setRedrawMode(RedrawMode mode) override;

  virtual void loseFocus() override;
  virtual void regainFocus() override;

protected:
  virtual void capture(void) override;
  virtual void releaseCapture(void) override;

  virtual UnsignedByte getMouseEvent(MouseIO *result, Bool flush) override;

  struct MacOSMouseEvent {
    int type;
    int x, y;
    int button;
    int wheelDelta;
    unsigned int time;
  };

  enum { MAX_EVENTS = 256 };
  MacOSMouseEvent m_eventBuffer[MAX_EVENTS];
  unsigned int m_nextFreeIndex;
  unsigned int m_nextGetIndex;

private:
  void initW3DAssets();
  void freeW3DAssets();

  void loadANICursors();
  void freeANICursors();
  int loadANIFrames(const char *path, NSCursor * __strong outCursors[], int maxFrames, int *outStepRatesMs, int *outCycleTotalMs);
  NSCursor *parseCURData(uint8_t *iconData, int iconSize, int *outHotX, int *outHotY);
  void setCursorDirection(MouseCursor cursor);

  CameraClass *m_w3dCamera;
  MouseCursor m_currentW3DCursor;
  RenderObjClass *m_cursorModels[NUM_MOUSE_CURSORS];
  HAnimClass *m_cursorAnims[NUM_MOUSE_CURSORS];
  bool m_w3dAssetsLoaded;

  NSCursor *m_nsCursors[NUM_MOUSE_CURSORS][MAX_2D_CURSOR_ANIM_FRAMES];
  int m_nsCursorFrameCount[NUM_MOUSE_CURSORS];
  int m_nsCursorStepRateMs[NUM_MOUSE_CURSORS][MAX_2D_CURSOR_ANIM_FRAMES];
  int m_nsCursorCycleMs[NUM_MOUSE_CURSORS];
  bool m_aniCursorsLoaded;
  int m_directionFrame;

public:
  void addEvent(int type, int x, int y, int button, int wheelDelta,
                unsigned int time);
};

enum MacOSMouseEventType {
  MACOS_MOUSE_MOVE,
  MACOS_MOUSE_LBUTTON_DOWN,
  MACOS_MOUSE_LBUTTON_UP,
  MACOS_MOUSE_LBUTTON_DBLCLK,
  MACOS_MOUSE_RBUTTON_DOWN,
  MACOS_MOUSE_RBUTTON_UP,
  MACOS_MOUSE_RBUTTON_DBLCLK,
  MACOS_MOUSE_MBUTTON_DOWN,
  MACOS_MOUSE_MBUTTON_UP,
  MACOS_MOUSE_WHEEL,
};
