#pragma once
// CefHost: process-wide CEF lifecycle + child-HWND host window class.
// Airspace note: CEF windowed rendering uses a real Win32 child HWND.
// That HWND always paints above XAML inside its rectangle, so no XAML
// overlay (flyouts/dialogs anchored inside the rect) can cover it.
// We therefore keep dialogs/flyouts in the toolbar/titlebar area and
// hide CEF HWNDs while a modal ContentDialog is open (see MainWindow).
#include <windows.h>
#include <string>
class CefBrowser;
namespace FluentBrowserCef
{
    // Returns >=0 exit code if this process is a CEF subprocess (caller must exit).
    int  HandleSubprocessIfNeeded(HINSTANCE instance);
    bool InitializeCef(HINSTANCE instance);
    void ShutdownCef();
    bool IsCefInitialized();
    // Registers/creates the static child window that CEF will parent its HWND into.
    HWND CreateHostWindow(HWND parent);
    std::wstring LocalAppDataDir(); // %LOCALAPPDATA%\FluentBrowser
}
