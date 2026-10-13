#pragma once

#include "afxwin.h"

#include <map>
#include <memory>
#include <vector>

#define WM_REFLECT_BASE 0xBC00

#define AFX_WND_REG 0x00001
#define AFX_WNDFRAMEORVIEW_REG 0x00008

extern const char kAfxWndClass[];
extern const char kAfxFrameOrViewClass[];

BOOL AFXAPI AfxDeferRegisterClass(LONG fToRegister);

class CControlBar;
class CImageList;

struct AfxThreadState
{
	CWnd* pWndInit = nullptr;
	HHOOK hHookOldCbtFilter = nullptr;
	MSG lastSentMsg = {};
	HWND hLockoutNotifyWindow = nullptr;
	HWND hTrackingWindow = nullptr;
	HMENU hTrackingMenu = nullptr;
	std::vector<CFrameWnd*> frames;
	int nLastStatus = -1;
	CControlBar* pLastStatus = nullptr;
};

AfxThreadState& AfxGetThreadState();

struct AfxModuleState
{
	CWinApp* pCurrentWinApp = nullptr;
	CWinThread* pCurrentWinThread = nullptr;
	HINSTANCE hCurrentInstanceHandle = nullptr;
	HINSTANCE hCurrentResourceHandle = nullptr;
	LPCSTR lpszCurrentAppName = nullptr;
};

AfxModuleState& AfxGetModuleState();

template<class Handle, class Object>
class AfxHandleMap
{
public:
	typedef Object* (*TemporaryFactory)(Handle handle);
	typedef void (*TemporaryDetacher)(Object* object);

	AfxHandleMap(TemporaryFactory createTemporary, TemporaryDetacher detachTemporary) :
		m_createTemporary(createTemporary),
		m_detachTemporary(detachTemporary)
	{
	}

	Object* fromHandle(Handle handle)
	{
		if (handle == nullptr)
		{
			return nullptr;
		}
		Object* object = lookupPermanent(handle);
		if (object != nullptr)
		{
			return object;
		}
		std::unique_ptr<Object>& temporary = m_temporary[handle];
		if (temporary == nullptr)
		{
			temporary.reset(m_createTemporary(handle));
		}
		return temporary.get();
	}

	Object* lookupPermanent(Handle handle) const
	{
		const auto found = m_permanent.find(handle);
		return found != m_permanent.end() ? found->second : nullptr;
	}

	void setPermanent(Handle handle, Object* object)
	{
		m_permanent[handle] = object;
	}

	void removeHandle(Handle handle)
	{
		m_permanent.erase(handle);
	}

	void deleteTemporaries()
	{
		std::map<Handle, std::unique_ptr<Object>> temporaries;
		temporaries.swap(m_temporary);
		for (auto& entry : temporaries)
		{
			m_detachTemporary(entry.second.get());
		}
	}

private:
	TemporaryFactory m_createTemporary;
	TemporaryDetacher m_detachTemporary;
	std::map<Handle, Object*> m_permanent;
	std::map<Handle, std::unique_ptr<Object>> m_temporary;
};

AfxHandleMap<HWND, CWnd>& AfxWindowMap();
AfxHandleMap<HMENU, CMenu>& AfxMenuMap();
AfxHandleMap<HDC, CDC>& AfxDeviceContextMap();
AfxHandleMap<HGDIOBJ, CGdiObject>& AfxGdiObjectMap();
AfxHandleMap<HIMAGELIST, CImageList>& AfxImageListMap();

void AFXAPI AfxDeleteTemporaryObjects();
HWND AFXAPI AfxGetParentOwner(HWND hWnd);
BOOL AFXAPI AfxIsDescendant(HWND hWndParent, HWND hWndChild);
