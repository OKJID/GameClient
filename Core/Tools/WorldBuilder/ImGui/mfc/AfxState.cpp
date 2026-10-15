#include "AfxInternal.h"

#include "afxcmn.h"

namespace
{

CWnd* createTemporaryWindow(HWND hWnd)
{
	CWnd* window = new CWnd;
	window->m_hWnd = hWnd;
	return window;
}

void detachTemporaryWindow(CWnd* window)
{
	window->m_hWnd = nullptr;
}

CMenu* createTemporaryMenu(HMENU hMenu)
{
	CMenu* menu = new CMenu;
	menu->m_hMenu = hMenu;
	return menu;
}

void detachTemporaryMenu(CMenu* menu)
{
	menu->m_hMenu = nullptr;
}

CDC* createTemporaryDeviceContext(HDC hDC)
{
	CDC* deviceContext = new CDC;
	deviceContext->m_hDC = hDC;
	deviceContext->m_hAttribDC = hDC;
	return deviceContext;
}

void detachTemporaryDeviceContext(CDC* deviceContext)
{
	deviceContext->m_hDC = nullptr;
	deviceContext->m_hAttribDC = nullptr;
}

CGdiObject* createTemporaryGdiObject(HGDIOBJ hObject)
{
	CGdiObject* object = new CGdiObject;
	object->m_hObject = hObject;
	return object;
}

void detachTemporaryGdiObject(CGdiObject* object)
{
	object->m_hObject = nullptr;
}

CImageList* createTemporaryImageList(HIMAGELIST hImageList)
{
	CImageList* imageList = new CImageList;
	imageList->m_hImageList = hImageList;
	return imageList;
}

void detachTemporaryImageList(CImageList* imageList)
{
	imageList->m_hImageList = nullptr;
}

}

AfxThreadState& AfxGetThreadState()
{
	static AfxThreadState state;
	return state;
}

AfxModuleState& AfxGetModuleState()
{
	static AfxModuleState state;
	return state;
}

AfxHandleMap<HWND, CWnd>& AfxWindowMap()
{
	static AfxHandleMap<HWND, CWnd> map(createTemporaryWindow, detachTemporaryWindow);
	return map;
}

AfxHandleMap<HMENU, CMenu>& AfxMenuMap()
{
	static AfxHandleMap<HMENU, CMenu> map(createTemporaryMenu, detachTemporaryMenu);
	return map;
}

AfxHandleMap<HDC, CDC>& AfxDeviceContextMap()
{
	static AfxHandleMap<HDC, CDC> map(createTemporaryDeviceContext, detachTemporaryDeviceContext);
	return map;
}

AfxHandleMap<HGDIOBJ, CGdiObject>& AfxGdiObjectMap()
{
	static AfxHandleMap<HGDIOBJ, CGdiObject> map(createTemporaryGdiObject, detachTemporaryGdiObject);
	return map;
}

AfxHandleMap<HIMAGELIST, CImageList>& AfxImageListMap()
{
	static AfxHandleMap<HIMAGELIST, CImageList> map(createTemporaryImageList, detachTemporaryImageList);
	return map;
}

void AFXAPI AfxDeleteTemporaryObjects()
{
	AfxWindowMap().deleteTemporaries();
	AfxMenuMap().deleteTemporaries();
	AfxDeviceContextMap().deleteTemporaries();
	AfxGdiObjectMap().deleteTemporaries();
	AfxImageListMap().deleteTemporaries();
}

HWND AFXAPI AfxGetParentOwner(HWND hWnd)
{
	if ((::GetWindowLongA(hWnd, GWL_STYLE) & WS_CHILD) != 0)
	{
		return ::GetParent(hWnd);
	}
	return ::GetWindow(hWnd, GW_OWNER);
}

BOOL AFXAPI AfxIsDescendant(HWND hWndParent, HWND hWndChild)
{
	for (HWND current = hWndChild; current != nullptr; current = AfxGetParentOwner(current))
	{
		if (current == hWndParent)
		{
			return TRUE;
		}
	}
	return FALSE;
}
