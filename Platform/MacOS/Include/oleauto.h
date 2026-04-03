#pragma once
#ifdef __APPLE__
// TODO(PS_PATH): OLE Automation stub — COM not available on macOS
typedef void* VARIANT;
typedef void* BSTR;
typedef void* IDispatch;
typedef void* ITypeInfo;
typedef void* ITypeLib;
typedef unsigned short VARTYPE;
typedef long DISPID;

#define DISPATCH_METHOD 0x1
#define DISPATCH_PROPERTYGET 0x2

inline BSTR SysAllocString(const wchar_t*) { return nullptr; }
inline void SysFreeString(BSTR) {}
#endif
