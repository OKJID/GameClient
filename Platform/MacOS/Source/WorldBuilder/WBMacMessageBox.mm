#include "WBMacCocoa.h"

#include "WBMacMessageBox.h"
#include "WBMacIdentity.h"

#include "Win32Api.h"

#include <algorithm>
#include <vector>

namespace
{

struct MessageBoxButton
{
	NSString* title;
	int result;
};

std::vector<MessageBoxButton> buttonsFor(UINT type)
{
	switch (type & MB_TYPEMASK)
	{
	case MB_OKCANCEL:
		return { { @"OK", IDOK }, { @"Cancel", IDCANCEL } };
	case MB_ABORTRETRYIGNORE:
		return { { @"Abort", IDABORT }, { @"Retry", IDRETRY }, { @"Ignore", IDIGNORE } };
	case MB_YESNOCANCEL:
		return { { @"Yes", IDYES }, { @"No", IDNO }, { @"Cancel", IDCANCEL } };
	case MB_YESNO:
		return { { @"Yes", IDYES }, { @"No", IDNO } };
	case MB_RETRYCANCEL:
		return { { @"Retry", IDRETRY }, { @"Cancel", IDCANCEL } };
	default:
		return { { @"OK", IDOK } };
	}
}

NSAlertStyle alertStyleFor(UINT type)
{
	switch (type & MB_ICONMASK)
	{
	case MB_ICONERROR:
		return NSAlertStyleCritical;
	case MB_ICONWARNING:
		return NSAlertStyleWarning;
	default:
		return NSAlertStyleInformational;
	}
}

NSString* stringFrom(const char* text)
{
	if (text == nullptr)
	{
		return @"";
	}
	NSString* result = [NSString stringWithUTF8String:text];
	return result != nil ? result : [NSString stringWithCString:text encoding:NSWindowsCP1252StringEncoding];
}

NSString* captionFrom(const char* caption)
{
	if (caption == nullptr || caption[0] == '\0')
	{
		return @"";
	}
	return [NSString stringWithFormat:@"%@ — %@", stringFrom(caption), stringFrom(WBMacIdentityName().c_str())];
}

void addButtons(NSAlert* alert, const std::vector<MessageBoxButton>& buttons, UINT type)
{
	const size_t defaultIndex = std::min<size_t>((type & 0x00000F00) >> 8, buttons.size() - 1);
	for (size_t index = 0; index < buttons.size(); ++index)
	{
		NSButton* button = [alert addButtonWithTitle:buttons[index].title];
		button.keyEquivalent = @"";
		if (buttons[index].result == IDCANCEL)
		{
			button.keyEquivalent = @"\033";
		}
		if (index == defaultIndex)
		{
			button.keyEquivalent = @"\r";
		}
	}
}

}

int WBMacMessageBox(HWND, const char* text, const char* caption, UINT type)
{
	@autoreleasepool
	{
		const std::vector<MessageBoxButton> buttons = buttonsFor(type);
		NSAlert* alert = [[NSAlert alloc] init];
		alert.messageText = captionFrom(caption);
		alert.informativeText = stringFrom(text);
		alert.alertStyle = alertStyleFor(type);
		alert.icon = NSApp.applicationIconImage;
		addButtons(alert, buttons, type);

		[NSApp activateIgnoringOtherApps:YES];
		const NSModalResponse response = [alert runModal];
		const size_t index = static_cast<size_t>(response - NSAlertFirstButtonReturn);
		return index < buttons.size() ? buttons[index].result : IDCANCEL;
	}
}
