#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "Browser/CefHost.h"
using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::FluentBrowser::implementation
{
    App::App()
    {
        InitializeComponent();
        UnhandledException({ this, &App::OnUnhandledException });
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        if (IsDebuggerPresent()) { auto e = DebugSettings(); e.IsBindingTracingEnabled(true); }
#endif
    }
    void App::OnUnhandledException(IInspectable const&, UnhandledExceptionEventArgs const& e) { e.Handled(true); }
    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        // CEF was initialized in wWinMain (subprocess check must happen there).
        window = make<MainWindow>();
        window.Activate();
    }
}

// Entry point. CEF subprocess handling MUST run before WinUI starts:
// same-exe pattern - CEF relaunches this exe with --type=... for its
// renderer/GPU/utility processes; CefExecuteProcess handles those and
// returns >= 0, in which case we exit immediately.
int __stdcall wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    int subprocess = FluentBrowserCef::HandleSubprocessIfNeeded(instance);
    if (subprocess >= 0) return subprocess;
    if (!FluentBrowserCef::InitializeCef(instance)) return 1;
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    ::winrt::Microsoft::UI::Xaml::Application::Start([](auto&&)
    {
        ::winrt::make<::winrt::FluentBrowser::implementation::App>();
    });
    FluentBrowserCef::ShutdownCef();
    return 0;
}

// C++/WinRT generated component sources. No .targets file adds these to the
// build, but they are required at link time:
//  - module.g.cpp defines WINRT_GetActivationFactory/WINRT_CanUnloadNow, which the
//    WinAppSDK 1.7 XAML compiler's XamlTypeInfo.g.cpp references (VSDesigner block).
//  - App.g.cpp / XamlMetaDataProvider.g.cpp define the activation factories that
//    module.g.cpp dispatches to (winrt_make_FluentBrowser_App / _XamlMetaDataProvider).
// Included here (same pattern as MainWindow.g.cpp in MainWindow.xaml.cpp) so no
// explicit ClCompile item for a not-yet-generated file is needed.
#if __has_include("module.g.cpp")
#include "module.g.cpp"
#endif
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif
#if __has_include("XamlMetaDataProvider.g.cpp")
#include "XamlMetaDataProvider.g.cpp"
#endif
