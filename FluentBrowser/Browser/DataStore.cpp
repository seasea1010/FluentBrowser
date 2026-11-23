#include "pch.h"
#include "DataStore.h"
#include "CefHost.h"
#include <fstream>
#include <sstream>
using namespace winrt;
using namespace Windows::Data::Json;

std::wstring AppSettingsData::SearchUrlFor(const std::wstring& q) const
{
    // Minimal percent-encoding for query text.
    std::wstring enc;
    for (wchar_t c : q)
    {
        if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'-' || c == L'_' || c == L'.' || c == L'~')
            enc += c;
        else if (c == L' ') enc += L"+";
        else { wchar_t buf[8]; swprintf_s(buf, L"%%%02X", (unsigned int)c); enc += buf; }
    }
    if (SearchEngine == L"Bing") return L"https://www.bing.com/search?q=" + enc;
    if (SearchEngine == L"DuckDuckGo") return L"https://duckduckgo.com/?q=" + enc;
    return L"https://www.google.com/search?q=" + enc;
}
std::wstring DataStore::FilePath() const
{
    return (std::filesystem::path(FluentBrowserCef::LocalAppDataDir()) / L"data.json").wstring();
}
static JsonArray EntriesToJson(const std::vector<EntryItem>& v)
{
    JsonArray arr;
    for (auto& e : v) { JsonObject o; o.SetNamedValue(L"title", JsonValue::CreateStringValue(e.Title)); o.SetNamedValue(L"url", JsonValue::CreateStringValue(e.Url)); arr.Append(o); }
    return arr;
}
static std::vector<EntryItem> EntriesFromJson(JsonObject const& root, wchar_t const* key)
{
    std::vector<EntryItem> out;
    auto v = root.GetNamedValue(key, nullptr);
    if (!v || v.ValueType() != JsonValueType::Array) return out;
    for (auto item : v.GetArray()) { auto o = item.GetObject(); out.push_back({ std::wstring(o.GetNamedString(L"title", L"")), std::wstring(o.GetNamedString(L"url", L"")) }); }
    return out;
}
void DataStore::Load()
{
    std::ifstream in(FilePath(), std::ios::binary);
    if (!in) return;
    std::stringstream ss; ss << in.rdbuf();
    std::string bytes = ss.str();
    JsonObject root;
    // Decode UTF-8 file bytes to a wide string for JsonObject::TryParse.
    int n = MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
    std::wstring wide(n, L'\0');
    if (n) MultiByteToWideChar(CP_UTF8, 0, bytes.data(), (int)bytes.size(), wide.data(), n);
    if (JsonObject::TryParse(hstring(wide), root))
    {
        auto s = root.GetNamedObject(L"settings", nullptr);
        if (s) { Settings.SearchEngine = s.GetNamedString(L"searchEngine", Settings.SearchEngine); Settings.Homepage = s.GetNamedString(L"homepage", Settings.Homepage); Settings.Theme = s.GetNamedString(L"theme", Settings.Theme); }
        Bookmarks = EntriesFromJson(root, L"bookmarks");
        History = EntriesFromJson(root, L"history");
    }
}
void DataStore::Save() const
{
    std::error_code ec; std::filesystem::create_directories(FluentBrowserCef::LocalAppDataDir(), ec);
    JsonObject root; JsonObject s;
    s.SetNamedValue(L"searchEngine", JsonValue::CreateStringValue(Settings.SearchEngine));
    s.SetNamedValue(L"homepage", JsonValue::CreateStringValue(Settings.Homepage));
    s.SetNamedValue(L"theme", JsonValue::CreateStringValue(Settings.Theme));
    root.SetNamedValue(L"settings", s);
    root.SetNamedValue(L"bookmarks", EntriesToJson(Bookmarks));
    root.SetNamedValue(L"history", EntriesToJson(History));
    std::wstring wide = root.Stringify().c_str();
    int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
    std::string bytes(n, '\0');
    if (n) WideCharToMultiByte(CP_UTF8, 0, wide.data(), (int)wide.size(), bytes.data(), n, nullptr, nullptr);
    std::ofstream out(FilePath(), std::ios::binary | std::ios::trunc); out << bytes;
}
void DataStore::AddHistory(const std::wstring& title, const std::wstring& url)
{
    if (url.empty() || url.rfind(L"data:", 0) == 0) return;
    History.erase(std::remove_if(History.begin(), History.end(), [&](EntryItem& e){ return e.Url == url; }), History.end());
    History.insert(History.begin(), { title.empty() ? url : title, url });
    if (History.size() > 500) History.resize(500);
    Save();
}
bool DataStore::IsBookmarked(const std::wstring& url) const
{
    return std::any_of(Bookmarks.begin(), Bookmarks.end(), [&](EntryItem const& e){ return e.Url == url; });
}
bool DataStore::ToggleBookmark(const std::wstring& title, const std::wstring& url)
{
    if (IsBookmarked(url)) { Bookmarks.erase(std::remove_if(Bookmarks.begin(), Bookmarks.end(), [&](EntryItem& e){ return e.Url == url; }), Bookmarks.end()); Save(); return false; }
    Bookmarks.push_back({ title.empty() ? url : title, url }); Save(); return true;
}
