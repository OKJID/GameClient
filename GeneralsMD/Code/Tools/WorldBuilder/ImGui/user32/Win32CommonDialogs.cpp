#include "Win32CommonDialogs.h"

namespace
{

DWORD& lastCommonDialogError()
{
	static DWORD error = 0;
	return error;
}

}

void Win32SetCommonDialogError(DWORD error)
{
	lastCommonDialogError() = error;
}

DWORD CommDlgExtendedError()
{
	return lastCommonDialogError();
}

BOOL PrintDlgA(LPPRINTDLG printDialog)
{
	if (printDialog == nullptr || printDialog->lStructSize != sizeof(PRINTDLGA))
	{
		lastCommonDialogError() = CDERR_STRUCTSIZE;
		return FALSE;
	}
	lastCommonDialogError() = PDERR_NODEFAULTPRN;
	if ((printDialog->Flags & PD_RETURNDEFAULT) == 0)
	{
		MessageBoxA(printDialog->hwndOwner,
			"Before you can perform printer-related tasks such as page setup or printing a document, you need to install a printer.",
			"Printers", MB_OK | MB_ICONEXCLAMATION);
	}
	return FALSE;
}
