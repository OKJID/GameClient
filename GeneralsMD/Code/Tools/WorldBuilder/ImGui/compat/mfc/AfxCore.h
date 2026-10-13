#pragma once

#include "Win32Api.h"

#define AFXAPI
#define AFX_DATA
#define AFX_CDECL
#define PASCAL
#define afx_msg

class CArchive;
class CDumpContext;
class CObject;

struct CRuntimeClass
{
	LPCSTR m_lpszClassName;
	int m_nObjectSize;
	UINT m_wSchema;
	CObject* (*m_pfnCreateObject)();
	const CRuntimeClass* m_pBaseClass;

	CObject* CreateObject() const;
	BOOL IsDerivedFrom(const CRuntimeClass* pBaseClass) const;
};

class CObject
{
public:
	virtual ~CObject() = default;

	virtual CRuntimeClass* GetRuntimeClass() const;
	virtual void Serialize(CArchive& ar);
#ifdef RTS_DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	BOOL IsKindOf(const CRuntimeClass* pClass) const;
	BOOL IsSerializable() const;

	static const CRuntimeClass classCObject;

protected:
	CObject() = default;

private:
	CObject(const CObject&) = delete;
	CObject& operator=(const CObject&) = delete;
};

#define RUNTIME_CLASS(class_name) (const_cast<CRuntimeClass*>(&class_name::class##class_name))

#define DECLARE_DYNAMIC(class_name) \
public: \
	static const CRuntimeClass class##class_name; \
	virtual CRuntimeClass* GetRuntimeClass() const override;

#define DECLARE_DYNCREATE(class_name) \
	DECLARE_DYNAMIC(class_name) \
	static CObject* PASCAL CreateObject();

#define IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, wSchema, pfnNew) \
	const CRuntimeClass class_name::class##class_name = { #class_name, static_cast<int>(sizeof(class class_name)), wSchema, pfnNew, RUNTIME_CLASS(base_class_name) }; \
	CRuntimeClass* class_name::GetRuntimeClass() const { return RUNTIME_CLASS(class_name); }

#define IMPLEMENT_DYNAMIC(class_name, base_class_name) \
	IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, nullptr)

#define IMPLEMENT_DYNCREATE(class_name, base_class_name) \
	CObject* PASCAL class_name::CreateObject() { return new class_name; } \
	IMPLEMENT_RUNTIMECLASS(class_name, base_class_name, 0xFFFF, class_name::CreateObject)

#define DYNAMIC_DOWNCAST(class_name, object) \
	(static_cast<class_name*>(AfxDynamicDownCast(RUNTIME_CLASS(class_name), object)))

#define STATIC_DOWNCAST(class_name, object) (static_cast<class_name*>(object))

CObject* AFXAPI AfxDynamicDownCast(CRuntimeClass* pClass, CObject* pObject);

BOOL AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine);
void AFX_CDECL AfxTrace(LPCSTR lpszFormat, ...);

#ifdef RTS_DEBUG

class CDumpContext
{
public:
	CDumpContext& operator<<(LPCSTR lpsz);
	CDumpContext& operator<<(const void* lp);
	CDumpContext& operator<<(const CObject* pOb);
	CDumpContext& operator<<(const CObject& ob);
	CDumpContext& operator<<(int n);
	CDumpContext& operator<<(UINT u);
	CDumpContext& operator<<(LONG l);
	CDumpContext& operator<<(DWORD dw);
	CDumpContext& operator<<(float f);
	CDumpContext& operator<<(double d);

	int GetDepth() const { return m_nDepth; }
	void SetDepth(int nNewDepth) { m_nDepth = nNewDepth; }

private:
	int m_nDepth = 0;
};

extern CDumpContext afxDump;

void AFXAPI AfxAssertValidObject(const CObject* pOb, LPCSTR lpszFileName, int nLine);

#define ASSERT(f) do { if (!(f) && AfxAssertFailedLine(__FILE__, __LINE__)) __builtin_debugtrap(); } while (0)
#define VERIFY(f) ASSERT(f)
#define ASSERT_VALID(pOb) (AfxAssertValidObject(pOb, __FILE__, __LINE__))
#define TRACE AfxTrace
#define TRACE0(sz) AfxTrace("%s", sz)
#define TRACE1(sz, p1) AfxTrace(sz, p1)
#define TRACE2(sz, p1, p2) AfxTrace(sz, p1, p2)
#define TRACE3(sz, p1, p2, p3) AfxTrace(sz, p1, p2, p3)

#else

#define ASSERT(f) ((void)0)
#define VERIFY(f) ((void)(f))
#define ASSERT_VALID(pOb) ((void)0)
#define TRACE(...) ((void)0)
#define TRACE0(sz) ((void)0)
#define TRACE1(sz, p1) ((void)0)
#define TRACE2(sz, p1, p2) ((void)0)
#define TRACE3(sz, p1, p2, p3) ((void)0)

#endif

#define ASSERT_KINDOF(class_name, object) ASSERT((object)->IsKindOf(RUNTIME_CLASS(class_name)))
