#pragma once
#include "MainWindow.xaml.g.h"
#include "Browser/BrowserClient.h"
#include "Browser/DataStore.h"
#include <map>
namespace winrt::FluentBrowser::implementation
{
    struct TabState
    {
        Microsoft::UI::Xaml::Controls::TabViewItem Item{ nullptr };
        CefRefPtr<BrowserClient> Client;
        HWND Host = nullptr;
        hstring Url;
        hstring Title{ L"新しいタブ" };
        bool Loading = false;
    };
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        // XAML handlers
        void OnWindowLoaded(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnHostSizeChanged(IInspectable const& sender, Microsoft::UI::Xaml::SizeChangedEventArgs const& args);
        void OnWindowClosed(IInspectable const& sender, Microsoft::UI::Xaml::WindowEventArgs const& args);
        void AddTab_Click(Microsoft::UI::Xaml::Controls::TabView const& sender, IInspectable const& args);
        void Tabs_SelectionChanged(IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void Tabs_TabCloseRequested(Microsoft::UI::Xaml::Controls::TabView const& sender, Microsoft::UI::Xaml::Controls::TabViewTabCloseRequestedEventArgs const& args);
        void Back_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Forward_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Reload_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Home_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void Favorite_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void AddressBox_KeyDown(IInspectable const& sender, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
        void NewTabMenu_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void FindMenu_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void DevToolsMenu_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void SettingsMenu_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void FindNext_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void FindPrev_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void FindClose_Click(IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void FindBox_KeyDown(IInspectable const& sender, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
        void HistoryList_SelectionChanged(IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
        void BookmarksList_SelectionChanged(IInspectable const& sender, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& args);
    private:
        HWND WindowHwnd();
        std::shared_ptr<TabState> CreateTab(hstring const& url);
        std::shared_ptr<TabState> CurrentTab();
        void LayoutBrowsers();
        void NavigateCurrent(hstring const& raw);
        hstring NormalizeOrSearch(hstring const& raw);
        void UpdateToolbar();
        void RefreshLists();
        void SetTabsVisible(bool visible);
        winrt::fire_and_forget ShowSettingsDialog();
        DataStore store_;
        std::vector<std::shared_ptr<TabState>> tabs_;
        Microsoft::UI::Dispatching::DispatcherQueue dispatcher_{ nullptr };
    };
}
namespace winrt::FluentBrowser::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
