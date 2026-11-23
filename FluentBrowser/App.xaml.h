#pragma once
#include "App.xaml.g.h"
namespace winrt::FluentBrowser::implementation
{
    struct App : AppT<App>
    {
        App();
        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);
        void OnUnhandledException(IInspectable const&, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e);
    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
