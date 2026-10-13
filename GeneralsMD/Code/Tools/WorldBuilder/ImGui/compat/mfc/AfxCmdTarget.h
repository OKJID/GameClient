#pragma once

#include "AfxFile.h"

class CCmdTarget;
class CCmdUI;
class CMenu;
class CWnd;

#define CN_COMMAND 0
#define CN_UPDATE_COMMAND_UI (static_cast<UINT>(-1))

struct AFX_CMDHANDLERINFO
{
	CCmdTarget* pTarget;
};

struct AFX_NOTIFY
{
	LRESULT* pResult;
	NMHDR* pNMHDR;
};

struct AfxMessageCall
{
	UINT nMessage = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;
	UINT nID = 0;
	void* pExtra = nullptr;
	LRESULT lResult = 0;
};

typedef BOOL (*AFX_PMSG)(CCmdTarget* pTarget, AfxMessageCall& call);

struct AFX_MSGMAP_ENTRY
{
	UINT nMessage;
	UINT nCode;
	UINT nID;
	UINT nLastID;
	AFX_PMSG pfn;
};

struct AFX_MSGMAP
{
	const AFX_MSGMAP* (*pfnGetBaseMap)();
	const AFX_MSGMAP_ENTRY* lpEntries;
};

const AFX_MSGMAP_ENTRY* AFXAPI AfxFindMessageEntry(const AFX_MSGMAP_ENTRY* lpEntry, UINT nMsg, UINT nCode, UINT nID);

#define DECLARE_MESSAGE_MAP() \
protected: \
	static const AFX_MSGMAP* PASCAL GetThisMessageMap(); \
	const AFX_MSGMAP* GetMessageMap() const override;

#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
	const AFX_MSGMAP* theClass::GetMessageMap() const { return GetThisMessageMap(); } \
	const AFX_MSGMAP* PASCAL theClass::GetThisMessageMap() \
	{ \
		using ThisClass [[maybe_unused]] = theClass; \
		using TheBaseClass = baseClass; \
		static const AFX_MSGMAP_ENTRY _messageEntries[] = \
		{

#define END_MESSAGE_MAP() \
			{ 0, 0, 0, 0, nullptr } \
		}; \
		static const AFX_MSGMAP messageMap = { &TheBaseClass::GetThisMessageMap, &_messageEntries[0] }; \
		return &messageMap; \
	}

#define AFX_MSGMAP_HANDLER(body) \
	[](CCmdTarget* pTarget, AfxMessageCall& call) -> BOOL \
	{ \
		ThisClass* pThis = static_cast<ThisClass*>(pTarget); \
		(void)pThis; \
		(void)call; \
		body \
	}

#define AFX_MSGMAP_ENTRY_OF(message, code, id, lastId, body) \
	{ static_cast<UINT>(message), static_cast<UINT>(code), static_cast<UINT>(id), static_cast<UINT>(lastId), AFX_MSGMAP_HANDLER(body) },

class CCmdTarget : public CObject
{
	DECLARE_DYNAMIC(CCmdTarget)

public:
	CCmdTarget() = default;

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

	void BeginWaitCursor();
	void EndWaitCursor();
	void RestoreWaitCursor();

protected:
	static const AFX_MSGMAP* PASCAL GetThisMessageMap();
	virtual const AFX_MSGMAP* GetMessageMap() const;
};

class CCmdUI
{
public:
	CCmdUI() = default;
	virtual ~CCmdUI() = default;

	virtual void Enable(BOOL bOn = TRUE);
	virtual void SetCheck(int nCheck = 1);
	virtual void SetRadio(BOOL bOn = TRUE);
	virtual void SetText(LPCSTR lpszText);

	void ContinueRouting() { m_bContinueRouting = TRUE; }
	BOOL DoUpdate(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);

	UINT m_nID = 0;
	UINT m_nIndex = 0;
	CMenu* m_pMenu = nullptr;
	CMenu* m_pSubMenu = nullptr;
	CWnd* m_pOther = nullptr;
	BOOL m_bEnableChanged = FALSE;
	BOOL m_bContinueRouting = FALSE;
	UINT m_nIndexMax = 0;
	CMenu* m_pParentMenu = nullptr;
};

class CWaitCursor
{
public:
	CWaitCursor();
	~CWaitCursor();

	void Restore();
};

BOOL AFXAPI AfxFinishUpdateCommandUI(CCmdUI* pCmdUI);
