#include "pch.h"
#include "CefHost.h"
#include "include/cef_app.h"
#include "include/cef_browser_process_handler.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"

namespace
{
    bool g_initialized = false;

    class SimpleApp : public CefApp, public CefBrowserProcessHandler
    {
    public:
        SimpleApp() = default; // DISALLOW_COPY_AND_ASSIGN suppresses the implicit default ctor
        CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
        void OnBeforeCommandLineProcessing(const CefString& process_type,
            CefRefPtr<CefCommandLine> command_line) override
        {
            if (process_type.empty()) // browser process only
            {
                // Japanese locale is also set via CefSettings.locale.
                command_line->AppendSwitchWithValue("lang", "ja");
            }
        }
        IMPLEMENT_REFCOUNTING(SimpleApp);
        DISALLOW_COPY_AND_ASSIGN(SimpleApp);
    };

    LRESULT CALLBACK HostWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

namespace FluentBrowserCef
{
    std::wstring LocalAppDataDir()
    {
        wchar_t buf[MAX_PATH]{};
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
        std::wstring base = (n > 0 && n < MAX_PATH) ? buf : L".";
        return (std::filesystem::path(base) / L"FluentBrowser").wstring();
    }

    int HandleSubprocessIfNeeded(HINSTANCE instance)
    {
        CefMainArgs mainArgs(instance);
        auto app = CefRefPtr<SimpleApp>(new SimpleApp());
        return CefExecuteProcess(mainArgs, app, nullptr);
    }

    bool InitializeCef(HINSTANCE instance)
    {
        if (g_initialized) return true;
        std::filesystem::path dir = LocalAppDataDir();
        std::error_code ec;
        std::filesystem::create_directories(dir / L"Cache", ec);

        CefMainArgs mainArgs(instance);
        auto app = CefRefPtr<SimpleApp>(new SimpleApp());
        CefSettings settings;
        settings.no_sandbox = true; // portable/unpackaged dev build: no CEF sandbox helper setup; documented in README
        settings.multi_threaded_message_loop = true; // CEF runs its own UI thread; we marshal back via DispatcherQueue
        settings.external_message_pump = false;
        CefString(&settings.cache_path) = (dir / L"Cache").wstring();
        CefString(&settings.log_file) = (dir / L"cef.log").wstring();
        settings.log_severity = LOGSEVERITY_WARNING;
        CefString(&settings.locale) = "ja";
        g_initialized = CefInitialize(mainArgs, settings, app, nullptr);
        return g_initialized;
    }

    void ShutdownCef()
    {
        if (g_initialized) { CefShutdown(); g_initialized = false; }
    }
    bool IsCefInitialized() { return g_initialized; }

    HWND CreateHostWindow(HWND parent)
    {
        static const wchar_t* kClass = L"FluentBrowserCefHostWnd";
        static bool registered = false;
        if (!registered)
        {
            WNDCLASSEXW wc{ sizeof(wc) };
            wc.lpfnWndProc = HostWndProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = kClass;
            wc.style = CS_HREDRAW | CS_VREDRAW;
            RegisterClassExW(&wc);
            registered = true;
        }
        return CreateWindowExW(0, kClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 0, 0, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    }
}
