#pragma once

#include "Win32Api.h"

#include <memory>
#include <string>
#include <vector>

struct _PSP
{
	PROPSHEETPAGEA page;
};

struct Win32PropertySheetPage
{
	PROPSHEETPAGEA page = {};
	HPROPSHEETPAGE handle = nullptr;
	HWND hwnd = nullptr;
	bool isChanged = false;
	bool isReleased = false;
};

struct Win32PropertySheetState
{
	PROPSHEETHEADERA header = {};
	std::vector<std::unique_ptr<Win32PropertySheetPage>> pages;
	HWND tab = nullptr;
	RECT pageRect = {};
	int activeIndex = -1;
	bool hasApplied = false;
	bool isOwnedByWindow = false;
};

namespace Win32PropertySheets
{
const UINT kTabControlId = 0x3020;

std::string resourceText(LPCSTR text);
std::string pageTitle(const PROPSHEETPAGEA& page);
SIZE largestPageSize(const Win32PropertySheetState& state);
LRESULT notifyPage(HWND sheet, HWND page, UINT code, LPARAM lParam);
bool killActivePage(HWND sheet, Win32PropertySheetState& state);
bool activatePage(HWND sheet, Win32PropertySheetState& state, int index);
void insertTab(HWND tab, int index, const PROPSHEETPAGEA& page);
void appendPage(Win32PropertySheetState& state, const PROPSHEETPAGEA& page, HPROPSHEETPAGE handle);
bool removePage(HWND sheet, Win32PropertySheetState& state, int index);
void destroyPages(Win32PropertySheetState& state);
int pageIndexOf(const Win32PropertySheetState& state, HWND page);
int pageIndexOf(const Win32PropertySheetState& state, HPROPSHEETPAGE handle);
}
