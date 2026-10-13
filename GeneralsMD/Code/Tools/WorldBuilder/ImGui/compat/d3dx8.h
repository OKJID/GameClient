#pragma once

#include_next <d3dx8.h>

#include "Win32Api.h"

struct ID3DXFont
{
	virtual ULONG AddRef() = 0;
	virtual ULONG Release() = 0;
	virtual HRESULT Begin() = 0;
	virtual INT DrawTextA(LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, D3DCOLOR Color) = 0;
	virtual HRESULT End() = 0;
	virtual HRESULT OnLostDevice() = 0;
	virtual HRESULT OnResetDevice() = 0;

protected:
	virtual ~ID3DXFont() = default;
};

typedef ID3DXFont* LPD3DXFONT;

HRESULT WINAPI D3DXCreateFont(IDirect3DDevice8* pDevice, HFONT hFont, LPD3DXFONT* ppFont);
