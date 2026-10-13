#include "Win32Controls.h"

#include "Win32Button.h"
#include "Win32Class.h"
#include "Win32ColorPicker.h"
#include "Win32ComboBox.h"
#include "Win32Dialog.h"
#include "Win32Edit.h"
#include "Win32ListBox.h"
#include "Win32ListView.h"
#include "Win32RichEdit.h"
#include "Win32Static.h"
#include "Win32StatusBar.h"
#include "Win32TabControl.h"
#include "Win32Toolbar.h"
#include "Win32Trackbar.h"
#include "Win32TreeView.h"

namespace
{

template<class Control>
void registerControl(const char* name)
{
	Win32ClassRegistry::instance().registerControlClass(name, Win32ControlWindowProc, Win32CreateControl<Control>, CS_DBLCLKS);
}

}

void Win32RegisterStandardClasses()
{
	if (Win32ClassRegistry::instance().find("Button") != nullptr)
	{
		return;
	}

	registerControl<Win32Button>("Button");
	registerControl<Win32Static>("Static");
	registerControl<Win32Edit>("Edit");
	registerControl<Win32ComboBox>("ComboBox");
	registerControl<Win32ListBox>("ListBox");
	registerControl<Win32Trackbar>(TRACKBAR_CLASSA);
	registerControl<Win32Progress>(PROGRESS_CLASSA);
	registerControl<Win32TreeView>(WC_TREEVIEWA);
	registerControl<Win32ListView>(WC_LISTVIEWA);
	registerControl<Win32TabControl>(WC_TABCONTROLA);
	registerControl<Win32RichEdit>(RICHEDIT_CLASSA);
	registerControl<Win32RichEdit>("RICHEDIT");
	registerControl<Win32Toolbar>(TOOLBARCLASSNAMEA);
	registerControl<Win32ColorPicker>(WIN32_COLORPICKER_CLASS);
	registerControl<Win32StatusBar>(STATUSCLASSNAMEA);
	Win32Dialogs::registerDialogClass();
}
