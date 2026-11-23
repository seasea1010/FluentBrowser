#pragma once
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_display_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_download_handler.h"
#include "include/cef_keyboard_handler.h"
#include "include/cef_context_menu_handler.h"
#include <functional>
#include <string>

// One BrowserClient per tab. UI callbacks are invoked on CEF threads;
// MainWindow implementations must marshal to DispatcherQueue.
struct BrowserCallbacks
{
    std::function<void(const std::wstring& url)> OnAddress;
    std::function<void(const std::wstring& title)> OnTitle;
    std::function<void(bool loading, bool canBack, bool canForward)> OnLoading;
    std::function<void()> OnBrowserClosed;
    std::function<void(const std::wstring& name, int percent)> OnDownload;
    std::function<void(const std::string& action)> OnShortcut; // "newtab","closetab","focusbar","reload","devtools","nexttab"
};

class BrowserClient : public CefClient,
    public CefLifeSpanHandler, public CefDisplayHandler, public CefLoadHandler,
    public CefDownloadHandler, public CefKeyboardHandler, public CefContextMenuHandler
{
public:
    explicit BrowserClient(BrowserCallbacks cb) : callbacks_(std::move(cb)) {}
    CefRefPtr<CefBrowser> Browser() const { return browser_; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }
    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    // LifeSpan
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
    // Display
    void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url) override;
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;
    // Load
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;
    // Download (CEF 131 signatures)
    bool CanDownload(CefRefPtr<CefBrowser> browser, const CefString& url, const CefString& request_method) override;
    void OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
        const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) override;
    void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
        CefRefPtr<CefDownloadItemCallback> callback) override;
    // Keyboard
    bool OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& event, CefEventHandle os_event) override;
    // Context menu: do nothing -> CEF default menu is used.
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;

private:
    BrowserCallbacks callbacks_;
    CefRefPtr<CefBrowser> browser_;
    IMPLEMENT_REFCOUNTING(BrowserClient);
    DISALLOW_COPY_AND_ASSIGN(BrowserClient);
};
