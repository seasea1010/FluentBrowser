#pragma once
#include "App.g.h"
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
namespace winrt::FluentBrowser::factory_implementation
{
    struct App : AppT<App, implementation::App> {};
}
