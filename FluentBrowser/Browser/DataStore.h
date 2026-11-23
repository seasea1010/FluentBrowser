#pragma once
#include <string>
#include <vector>
struct EntryItem { std::wstring Title; std::wstring Url; };
struct AppSettingsData
{
    std::wstring SearchEngine = L"Google"; // Google | Bing | DuckDuckGo
    std::wstring Homepage = L"https://www.google.com/";
    std::wstring Theme = L"Default";       // Default | Light | Dark
    std::wstring SearchUrlFor(const std::wstring& query) const;
};
// Persists settings/bookmarks/history as JSON under %LOCALAPPDATA%\FluentBrowser\
// using Windows.Data.Json (no third-party dependency).
class DataStore
{
public:
    void Load();
    void Save() const;
    AppSettingsData Settings;
    std::vector<EntryItem> Bookmarks;
    std::vector<EntryItem> History; // newest first, capped at 500
    void AddHistory(const std::wstring& title, const std::wstring& url);
    bool ToggleBookmark(const std::wstring& title, const std::wstring& url); // returns true if now bookmarked
    bool IsBookmarked(const std::wstring& url) const;
private:
    std::wstring FilePath() const;
};
