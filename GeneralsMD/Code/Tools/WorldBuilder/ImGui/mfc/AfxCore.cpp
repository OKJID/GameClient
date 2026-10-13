#include "afx.h"

#include <cstdarg>
#include <cstdio>

const CRuntimeClass CObject::classCObject = { "CObject", static_cast<int>(sizeof(CObject)), 0xFFFF, nullptr, nullptr };

CObject* CRuntimeClass::CreateObject() const
{
	return m_pfnCreateObject != nullptr ? m_pfnCreateObject() : nullptr;
}

BOOL CRuntimeClass::IsDerivedFrom(const CRuntimeClass* pBaseClass) const
{
	for (const CRuntimeClass* current = this; current != nullptr; current = current->m_pBaseClass)
	{
		if (current == pBaseClass)
		{
			return TRUE;
		}
	}
	return FALSE;
}

CRuntimeClass* CObject::GetRuntimeClass() const
{
	return RUNTIME_CLASS(CObject);
}

void CObject::Serialize(CArchive&)
{
}

BOOL CObject::IsKindOf(const CRuntimeClass* pClass) const
{
	return GetRuntimeClass()->IsDerivedFrom(pClass);
}

BOOL CObject::IsSerializable() const
{
	return GetRuntimeClass()->m_wSchema != 0xFFFF ? TRUE : FALSE;
}

CObject* AFXAPI AfxDynamicDownCast(CRuntimeClass* pClass, CObject* pObject)
{
	return pObject != nullptr && pObject->IsKindOf(pClass) ? pObject : nullptr;
}

BOOL AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine)
{
	char message[1024];
	snprintf(message, sizeof(message), "Assertion failed: %s, line %d", lpszFileName, nLine);
	fprintf(stderr, "%s\n", message);
	return MessageBoxA(nullptr, message, "MFC Assertion", MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY ? TRUE : FALSE;
}

void AFX_CDECL AfxTrace(LPCSTR lpszFormat, ...)
{
	va_list arguments;
	va_start(arguments, lpszFormat);
	vfprintf(stderr, lpszFormat, arguments);
	va_end(arguments);
}

#ifdef RTS_DEBUG

CDumpContext afxDump;

void CObject::AssertValid() const
{
}

void CObject::Dump(CDumpContext& dc) const
{
	dc << "a " << GetRuntimeClass()->m_lpszClassName << " at " << static_cast<const void*>(this) << "\n";
}

void AFXAPI AfxAssertValidObject(const CObject* pOb, LPCSTR lpszFileName, int nLine)
{
	if (pOb == nullptr)
	{
		if (AfxAssertFailedLine(lpszFileName, nLine))
		{
			__builtin_debugtrap();
		}
		return;
	}
	pOb->AssertValid();
}

CDumpContext& CDumpContext::operator<<(LPCSTR lpsz)
{
	fputs(lpsz != nullptr ? lpsz : "(NULL)", stderr);
	return *this;
}

CDumpContext& CDumpContext::operator<<(const void* lp)
{
	fprintf(stderr, "$%p", lp);
	return *this;
}

CDumpContext& CDumpContext::operator<<(const CObject* pOb)
{
	if (pOb == nullptr)
	{
		return *this << "NULL";
	}
	pOb->Dump(*this);
	return *this;
}

CDumpContext& CDumpContext::operator<<(const CObject& ob)
{
	return *this << &ob;
}

CDumpContext& CDumpContext::operator<<(int n)
{
	fprintf(stderr, "%d", n);
	return *this;
}

CDumpContext& CDumpContext::operator<<(UINT u)
{
	fprintf(stderr, "%u", u);
	return *this;
}

CDumpContext& CDumpContext::operator<<(LONG l)
{
	fprintf(stderr, "%ld", static_cast<long>(l));
	return *this;
}

CDumpContext& CDumpContext::operator<<(DWORD dw)
{
	fprintf(stderr, "%lu", static_cast<unsigned long>(dw));
	return *this;
}

CDumpContext& CDumpContext::operator<<(float f)
{
	fprintf(stderr, "%f", static_cast<double>(f));
	return *this;
}

CDumpContext& CDumpContext::operator<<(double d)
{
	fprintf(stderr, "%f", d);
	return *this;
}

#endif
