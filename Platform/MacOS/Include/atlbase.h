#pragma once
#ifdef __APPLE__
// ATL (Active Template Library) is not available on macOS.
// WebBrowser.h includes this but the COM-based browser is never used on macOS.
#endif
