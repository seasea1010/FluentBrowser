#include "pch.h"
#include "BrowserClient.h"
#include "include/cef_download_handler.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_helpers.h"
#include <shlobj.h>

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) { browser_ = browser; }
bool BrowserClient::DoClose(CefRefPtr<CefBrowser>) { return false; }
void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser>)
{
    browser_ = nullptr;
    if (callbacks_.OnBrowserClosed) callbacks_.OnBrowserClosed();
}
void BrowserClient::OnAddressChange(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, const CefString& url)
{
    if (frame->IsMain() && callbacks_.OnAddress) callbacks_.OnAddress(url.ToWString());
}
void BrowserClient::OnTitleChange(CefRefPtr<CefBrowser>, const CefString& title)
{
    if (callbacks_.OnTitle) callbacks_.OnTitle(title.ToWString());
}
void BrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser>, bool isLoading, bool canGoBack, bool canGoForward)
{
    if (callbacks_.OnLoading) callbacks_.OnLoading(isLoading, canGoBack, canGoForward);
}
void BrowserClient::OnLoadError(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, ErrorCode, const CefString& errorText, const CefString& failedUrl)
{
    if (!frame->IsMain()) return;
    std::string html = "<html><head><meta charset='utf-8'><title>ページを開けません</title></head><body style='font-family:sans-serif;padding:32px'>"
        "<h2>ページを開けませんでした</h2><p>" + failedUrl.ToString() + "</p><p>" + errorText.ToString() + "</p></body></html>";
    frame->LoadURL("data:text/html;charset=utf-8," + html);
}
bool BrowserClient::CanDownload(CefRefPtr<CefBrowser>, const CefString&, const CefString&) { return true; }
bool BrowserClient::OnBeforeDownload(CefRefPtr<CefBrowser>, CefRefPtr<CefDownloadItem>,
    const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
    wchar_t path[MAX_PATH]{};
    std::wstring dir = L".";
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, path)))
        dir = std::filesystem::path(path) / L"Downloads";
    std::filesystem::path target = std::filesystem::path(dir) / suggested_name.ToWString();
    callback->Continue(target.wstring(), false); // no save-as dialog in v0.1
    if (callbacks_.OnDownload) callbacks_.OnDownload(suggested_name.ToWString(), 0);
    return true; // callback executed: continue the download (CEF 131)
}
void BrowserClient::OnDownloadUpdated(CefRefPtr<CefBrowser>, CefRefPtr<CefDownloadItem> item, CefRefPtr<CefDownloadItemCallback>)
{
    if (callbacks_.OnDownload) callbacks_.OnDownload(item->GetSuggestedFileName().ToWString(), item->GetPercentComplete());
}
bool BrowserClient::OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& e, CefEventHandle)
{
    if (e.type != KEYEVENT_RAWKEYDOWN) return false;
    bool ctrl = (e.modifiers & EVENTFLAG_CONTROL_DOWN) != 0;
    auto fire = [&](const char* a){ if (callbacks_.OnShortcut) callbacks_.OnShortcut(a); return true; };
    if (e.windows_key_code == VK_F12) { browser->GetHost()->ShowDevTools(CefWindowInfo{}, nullptr, CefBrowserSettings{}, CefPoint{}); return true; }
    if (!ctrl) return false;
    switch (e.windows_key_code)
    {
    case 'T': return fire("newtab");
    case 'W': return fire("closetab");
    case 'L': return fire("focusbar");
    case 'R': return fire("reload");
    case VK_TAB: return fire("nexttab");
    default: return false;
    }
}
void BrowserClient::OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefContextMenuParams>, CefRefPtr<CefMenuModel>)
{
    // Intentionally empty: keep CEF's default context menu.
}
