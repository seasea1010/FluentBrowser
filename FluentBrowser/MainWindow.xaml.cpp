#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "Browser/CefHost.h"
#include "include/cef_browser.h"
#include "include/cef_request_context.h"
using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Windows::System;

namespace winrt::FluentBrowser::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        Title(L"FluentBrowser");
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        store_.Load();
        // Apply saved theme to root element.
        auto theme = ElementTheme::Default;
        if (store_.Settings.Theme == L"Light") theme = ElementTheme::Light;
        else if (store_.Settings.Theme == L"Dark") theme = ElementTheme::Dark;
        RootGrid().RequestedTheme(theme);
        dispatcher_ = DispatcherQueue();
    }

    HWND MainWindow::WindowHwnd()
    {
        HWND hwnd{}; Window window = *this; check_hresult(window.as<IWindowNative>()->get_WindowHandle(&hwnd)); return hwnd;
    }

    void MainWindow::OnWindowLoaded(IInspectable const&, RoutedEventArgs const&)
    {
        RefreshLists();
        CreateTab(hstring(store_.Settings.Homepage));
    }
    void MainWindow::OnHostSizeChanged(IInspectable const&, SizeChangedEventArgs const&) { LayoutBrowsers(); }
    void MainWindow::OnWindowClosed(IInspectable const&, WindowEventArgs const&)
    {
        for (auto& t : tabs_) if (t->Client && t->Client->Browser()) t->Client->Browser()->GetHost()->CloseBrowser(true);
        tabs_.clear();
    }

    std::shared_ptr<TabState> MainWindow::CurrentTab()
    {
        int i = BrowserTabs().SelectedIndex();
        if (i >= 0 && i < (int)tabs_.size()) return tabs_[i];
        return nullptr;
    }

    std::shared_ptr<TabState> MainWindow::CreateTab(hstring const& url)
    {
        auto tab = std::make_shared<TabState>();
        tab->Url = url;
        TabViewItem item;
        item.Header(box_value(hstring(L"新しいタブ")));
        SymbolIconSource icon; icon.Symbol(Symbol::Globe); item.IconSource(icon);
        tab->Item = item;
        tab->Host = FluentBrowserCef::CreateHostWindow(WindowHwnd());

        BrowserCallbacks cb;
        auto* self = this;
        cb.OnAddress = [self, tab](std::wstring const& u) {
            self->dispatcher_.TryEnqueue([self, tab, u]() {
                tab->Url = hstring(u);
                if (self->CurrentTab() == tab) { self->AddressBox().Text(tab->Url); self->UpdateToolbar(); }
                self->store_.AddHistory(std::wstring(tab->Title.c_str()), u); self->RefreshLists();
            });
        };
        cb.OnTitle = [self, tab](std::wstring const& t) {
            self->dispatcher_.TryEnqueue([tab, t]() {
                tab->Title = hstring(t.empty() ? std::wstring(L"新しいタブ") : t);
                tab->Item.Header(box_value(tab->Title));
            });
        };
        cb.OnLoading = [self, tab](bool loading, bool, bool) {
            self->dispatcher_.TryEnqueue([self, tab, loading]() {
                tab->Loading = loading;
                if (self->CurrentTab() == tab) { self->StatusText().Text(loading ? L"読み込み中…" : L"完了"); self->UpdateToolbar(); }
            });
        };
        cb.OnDownload = [self](std::wstring const& name, int percent) {
            self->dispatcher_.TryEnqueue([self, name, percent]() {
                self->DownloadsList().Items().Append(box_value(hstring(name) + L" — " + to_hstring(percent) + L"%"));
                self->StatusText().Text(L"ダウンロード: " + hstring(name));
            });
        };
        cb.OnShortcut = [self](std::string const& a) {
            self->dispatcher_.TryEnqueue([self, a]() {
                if (a == "newtab") self->CreateTab(hstring(self->store_.Settings.Homepage));
                else if (a == "closetab") { if (auto t = self->CurrentTab()) { int i = self->BrowserTabs().SelectedIndex(); self->Tabs_TabCloseRequested(self->BrowserTabs(), nullptr); (void)i; (void)t; } }
                else if (a == "focusbar") { self->AddressBox().Focus(FocusState::Keyboard); self->AddressBox().SelectAll(); }
                else if (a == "reload") { if (auto t = self->CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->Reload(); }
                else if (a == "nexttab") { int n = (int)self->tabs_.size(); if (n) self->BrowserTabs().SelectedIndex((self->BrowserTabs().SelectedIndex() + 1) % n); }
                else if (a == "devtools") { if (auto t = self->CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GetHost()->ShowDevTools(CefWindowInfo{}, nullptr, CefBrowserSettings{}, CefPoint{}); }
            });
        };
        cb.OnBrowserClosed = []() {};
        tab->Client = new BrowserClient(std::move(cb));
        tabs_.push_back(tab);
        BrowserTabs().TabItems().Append(item);
        BrowserTabs().SelectedItem(item);

        CefWindowInfo wi;
        RECT rc{ 0, 0, 800, 600 };
        wi.SetAsChild(tab->Host, CefRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top));
        CefBrowserSettings bs;
        CefBrowserHost::CreateBrowser(wi, tab->Client, std::string(winrt::to_string(url)), bs, nullptr, nullptr);
        LayoutBrowsers();
        return tab;
    }

    void MainWindow::LayoutBrowsers()
    {
        // Convert BrowserHostGrid rect (XAML DIP, relative to window content)
        // to window client pixels: point is already relative to RootGrid which
        // fills the client area; scale by RasterizationScale for DPI.
        if (!BrowserHostGrid() || tabs_.empty()) return;
        double scale = 1.0;
        if (auto xr = BrowserHostGrid().XamlRoot()) scale = xr.RasterizationScale();
        auto transform = BrowserHostGrid().TransformToVisual(RootGrid());
        auto pt = transform.TransformPoint(Windows::Foundation::Point(0, 0));
        int x = (int)std::lround(pt.X * scale), y = (int)std::lround(pt.Y * scale);
        int wpx = (int)std::lround(BrowserHostGrid().ActualWidth() * scale);
        int hpx = (int)std::lround(BrowserHostGrid().ActualHeight() * scale);
        auto cur = CurrentTab();
        for (auto& t : tabs_)
        {
            if (!t->Host) continue;
            bool active = (t == cur);
            ShowWindow(t->Host, active ? SW_SHOW : SW_HIDE);
            if (active) SetWindowPos(t->Host, HWND_TOP, x, y, (std::max)(wpx, 1), (std::max)(hpx, 1), SWP_NOACTIVATE);
            if (active && t->Client && t->Client->Browser())
            {
                HWND bh = t->Client->Browser()->GetHost()->GetWindowHandle();
                if (bh) SetWindowPos(bh, HWND_TOP, 0, 0, (std::max)(wpx, 1), (std::max)(hpx, 1), SWP_NOACTIVATE | SWP_NOMOVE);
                // CEF child fills host: also move host's child to host size (SetAsChild used host as parent via wrapper window)
                if (bh && GetParent(bh) != t->Host) SetWindowPos(bh, HWND_TOP, x, y, (std::max)(wpx, 1), (std::max)(hpx, 1), SWP_NOACTIVATE);
            }
        }
    }

    void MainWindow::SetTabsVisible(bool visible)
    {
        for (auto& t : tabs_) if (t->Host) ShowWindow(t->Host, visible && t == CurrentTab() ? SW_SHOW : SW_HIDE);
        if (visible) LayoutBrowsers();
    }

    hstring MainWindow::NormalizeOrSearch(hstring const& raw)
    {
        std::wstring s = raw.c_str();
        auto trim = [](std::wstring v) { size_t a = v.find_first_not_of(L" \t\r\n"); size_t b = v.find_last_not_of(L" \t\r\n"); return a == std::wstring::npos ? std::wstring{} : v.substr(a, b - a + 1); };
        s = trim(s);
        if (s.find(L"://") != std::wstring::npos || s.rfind(L"about:", 0) == 0 || s.rfind(L"data:", 0) == 0) return hstring(s);
        bool hasSpace = s.find(L' ') != std::wstring::npos;
        bool looksLikeHost = !hasSpace && s.find(L'.') != std::wstring::npos;
        if (looksLikeHost) return hstring(L"https://" + s);
        return hstring(store_.Settings.SearchUrlFor(s));
    }
    void MainWindow::NavigateCurrent(hstring const& raw)
    {
        auto t = CurrentTab(); if (!t || !t->Client || !t->Client->Browser()) return;
        auto url = NormalizeOrSearch(raw);
        t->Client->Browser()->GetMainFrame()->LoadURL(std::string(to_string(url)));
    }
    void MainWindow::UpdateToolbar()
    {
        auto t = CurrentTab();
        bool back = false, fwd = false;
        if (t && t->Client && t->Client->Browser()) { back = t->Client->Browser()->CanGoBack(); fwd = t->Client->Browser()->CanGoForward(); }
        BackBtn().IsEnabled(back); ForwardBtn().IsEnabled(fwd);
    }
    void MainWindow::RefreshLists()
    {
        HistoryList().Items().Clear();
        for (auto& e : store_.History) HistoryList().Items().Append(box_value(hstring(e.Title) + L"\n" + hstring(e.Url)));
        BookmarksList().Items().Clear();
        for (auto& e : store_.Bookmarks) BookmarksList().Items().Append(box_value(hstring(e.Title) + L"\n" + hstring(e.Url)));
    }

    // ---- XAML handlers ----
    void MainWindow::AddTab_Click(TabView const&, IInspectable const&) { CreateTab(hstring(store_.Settings.Homepage)); }
    void MainWindow::NewTabMenu_Click(IInspectable const&, RoutedEventArgs const&) { CreateTab(hstring(store_.Settings.Homepage)); }
    void MainWindow::Tabs_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        if (auto t = CurrentTab()) AddressBox().Text(t->Url);
        UpdateToolbar(); LayoutBrowsers();
    }
    void MainWindow::Tabs_TabCloseRequested(TabView const&, TabViewTabCloseRequestedEventArgs const& args)
    {
        // When called from keyboard shortcut, args may be null.
        int idx = BrowserTabs().SelectedIndex();
        if (args) { auto items = BrowserTabs().TabItems(); for (int i = 0; i < (int)items.Size(); ++i) if (items.GetAt(i) == args.Tab()) idx = i; }
        if (idx < 0 || idx >= (int)tabs_.size()) return;
        auto tab = tabs_[idx];
        if (tab->Client && tab->Client->Browser()) tab->Client->Browser()->GetHost()->CloseBrowser(true);
        if (tab->Host) DestroyWindow(tab->Host);
        BrowserTabs().TabItems().RemoveAt(idx);
        tabs_.erase(tabs_.begin() + idx);
        if (tabs_.empty()) CreateTab(hstring(store_.Settings.Homepage)); else LayoutBrowsers();
    }
    void MainWindow::Back_Click(IInspectable const&, RoutedEventArgs const&) { if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GoBack(); }
    void MainWindow::Forward_Click(IInspectable const&, RoutedEventArgs const&) { if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GoForward(); }
    void MainWindow::Reload_Click(IInspectable const&, RoutedEventArgs const&) { if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) { if (t->Loading) t->Client->Browser()->StopLoad(); else t->Client->Browser()->Reload(); } }
    void MainWindow::Home_Click(IInspectable const&, RoutedEventArgs const&) { NavigateCurrent(hstring(store_.Settings.Homepage)); }
    void MainWindow::Favorite_Click(IInspectable const&, RoutedEventArgs const&)
    {
        auto t = CurrentTab(); if (!t) return;
        bool now = store_.ToggleBookmark(std::wstring(t->Title.c_str()), std::wstring(t->Url.c_str()));
        StatusText().Text(now ? L"お気に入りに追加しました" : L"お気に入りから削除しました");
        RefreshLists();
    }
    void MainWindow::AddressBox_KeyDown(IInspectable const&, KeyRoutedEventArgs const& args)
    {
        if (args.Key() == VirtualKey::Enter) NavigateCurrent(AddressBox().Text());
    }
    void MainWindow::FindMenu_Click(IInspectable const&, RoutedEventArgs const&) { FindBar().Visibility(Visibility::Visible); FindBox().Focus(FocusState::Keyboard); }
    void MainWindow::DevToolsMenu_Click(IInspectable const&, RoutedEventArgs const&) { if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GetHost()->ShowDevTools(CefWindowInfo{}, nullptr, CefBrowserSettings{}, CefPoint{}); }
    void MainWindow::FindClose_Click(IInspectable const&, RoutedEventArgs const&)
    {
        FindBar().Visibility(Visibility::Collapsed);
        if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GetHost()->StopFinding(true);
    }
    void MainWindow::FindNext_Click(IInspectable const&, RoutedEventArgs const&) { if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GetHost()->Find(std::string(to_string(FindBox().Text())), true, false, false); }
    void MainWindow::FindPrev_Click(IInspectable const&, RoutedEventArgs const&) { if (auto t = CurrentTab(); t && t->Client && t->Client->Browser()) t->Client->Browser()->GetHost()->Find(std::string(to_string(FindBox().Text())), false, false, false); }
    void MainWindow::FindBox_KeyDown(IInspectable const&, KeyRoutedEventArgs const& args) { if (args.Key() == VirtualKey::Enter) FindNext_Click(nullptr, nullptr); }
    void MainWindow::HistoryList_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        int i = HistoryList().SelectedIndex(); if (i >= 0 && i < (int)store_.History.size()) NavigateCurrent(hstring(store_.History[i].Url));
    }
    void MainWindow::BookmarksList_SelectionChanged(IInspectable const&, SelectionChangedEventArgs const&)
    {
        int i = BookmarksList().SelectedIndex(); if (i >= 0 && i < (int)store_.Bookmarks.size()) NavigateCurrent(hstring(store_.Bookmarks[i].Url));
    }
    void MainWindow::SettingsMenu_Click(IInspectable const&, RoutedEventArgs const&) { ShowSettingsDialog(); }

    winrt::fire_and_forget MainWindow::ShowSettingsDialog()
    {
        auto lifetime = get_strong();
        SetTabsVisible(false); // CEF HWND would paint over the dialog (airspace)
        ContentDialog dlg; dlg.XamlRoot(RootGrid().XamlRoot()); dlg.Title(box_value(hstring(L"設定")));
        StackPanel panel; panel.Spacing(12); panel.Width(360);
        TextBlock engLabel; engLabel.Text(L"既定の検索エンジン");
        ComboBox eng; eng.Items().Append(box_value(hstring(L"Google"))); eng.Items().Append(box_value(hstring(L"Bing"))); eng.Items().Append(box_value(hstring(L"DuckDuckGo")));
        eng.SelectedIndex(store_.Settings.SearchEngine == L"Bing" ? 1 : store_.Settings.SearchEngine == L"DuckDuckGo" ? 2 : 0);
        TextBlock homeLabel; homeLabel.Text(L"ホームページ");
        TextBox home; home.Text(hstring(store_.Settings.Homepage));
        TextBlock themeLabel; themeLabel.Text(L"テーマ");
        ComboBox theme; theme.Items().Append(box_value(hstring(L"既定"))); theme.Items().Append(box_value(hstring(L"ライト"))); theme.Items().Append(box_value(hstring(L"ダーク")));
        theme.SelectedIndex(store_.Settings.Theme == L"Light" ? 1 : store_.Settings.Theme == L"Dark" ? 2 : 0);
        panel.Children().Append(engLabel); panel.Children().Append(eng);
        panel.Children().Append(homeLabel); panel.Children().Append(home);
        panel.Children().Append(themeLabel); panel.Children().Append(theme);
        dlg.Content(panel); dlg.PrimaryButtonText(L"保存"); dlg.CloseButtonText(L"キャンセル");
        auto result = co_await dlg.ShowAsync();
        if (result == ContentDialogResult::Primary)
        {
            store_.Settings.SearchEngine = eng.SelectedIndex() == 1 ? L"Bing" : eng.SelectedIndex() == 2 ? L"DuckDuckGo" : L"Google";
            store_.Settings.Homepage = home.Text().c_str();
            store_.Settings.Theme = theme.SelectedIndex() == 1 ? L"Light" : theme.SelectedIndex() == 2 ? L"Dark" : L"Default";
            store_.Save();
            RootGrid().RequestedTheme(store_.Settings.Theme == L"Light" ? ElementTheme::Light : store_.Settings.Theme == L"Dark" ? ElementTheme::Dark : ElementTheme::Default);
        }
        SetTabsVisible(true);
    }
}
