# FluentBrowser

WinUI 3（Windows App SDK、C++/WinRT）+ CEF（Chromium Embedded Framework）で作る、
**本物の Fluent UI** の Windows 向けブラウザです。Lumo とは別の新規プロジェクトです。

## 「ガチの Fluent」とは

- 見た目を Fluent 風に寄せたものではありません。Microsoft 公式の WinUI 3 コントロール
  （`TabView` / `AppBarButton` / `MenuFlyout` / `ContentDialog` / `ListView`）そのものを使います。
- 背景は WinUI 3 標準の **Mica**（`MicaBackdrop`）。色はシステムアクセントとテーマに追従します。
- 独自のグラデーション、グロー、ツヤといった装飾は入れていません。フラットで自然な標準デザインです。
- ブラウザの形は Edge と同じ流儀です：タブは `TabView`、その下にツールバー。
  Web コンテンツ用に `NavigationView` は使いません。

## アーキテクチャ

```
FluentBrowser.exe (WinUI 3 / C++/WinRT, unpackaged・自己完結)
├─ MainWindow.xaml        … Fluent UI（Mica、カスタムタイトルバー、TabView、ツールバー、検索バー）
├─ Browser/CefHost        … CEF 初期化・終了、サブプロセス判定、子 HWND ホスト
├─ Browser/BrowserClient  … タブごとの CefClient（表示/読込/ダウンロード/キー/メニュー）
└─ Browser/DataStore      … 設定・お気に入り・履歴を JSON 保存 (Windows.Data.Json)
     保存先: %LOCALAPPDATA%\FluentBrowser\ (data.json / Cache\ / cef.log)
```

### CEF の載せ方と「エアスペース」の注意

CEF はウィンドウモードで動かし、その描画先は Win32 の子 HWND です。
WinUI 3 ウィンドウの HWND（`IWindowNative` で取得）の中に専用の子ウィンドウ
（`FluentBrowserCefHostWnd`）を作り、タブごとに CEF ブラウザをそこへ載せます。
配置は XAML 側のプレースホルダ `BrowserHostGrid` の位置・サイズを
DPI スケール（`RasterizationScale`）込みでクライアント座標へ変換し、
`Loaded` / `SizeChanged` のたびに `SetWindowPos` で同期します。

子 HWND はその矩形内では XAML より常に前面に描かれます（Win32 と XAML のエアスペース問題）。
そのため、矩形の上に XAML のダイアログ等を重ねることはできません。
v0.1 では設定 `ContentDialog` を開く間だけ CEF の HWND を隠して回避しています。
Flyout はツールバー側（矩形の外）に出るので問題ありません。

### CEF サブプロセス

別 exe は用意せず、同じ exe を使う方式です。`wWinMain` の最初（WinUI 起動前）に
`CefExecuteProcess` を呼び、戻り値が 0 以上ならそのまま終了します。
これで CEF が `--type=...` 付きで再起動した子プロセスを正しく処理します。

CEF 設定：`multi_threaded_message_loop = true`、`locale = ja`、
キャッシュは `%LOCALAPPDATA%\FluentBrowser\Cache`。
`no_sandbox = true` にしています（ポータブルな unpackaged 構成では CEF サンドボックスの
ヘルパー配置が別途必要になるため。個人用 v0.1 の割り切りで、本番配布時は要見直し）。

## ビルド方法

### GitHub Actions（おすすめ・手元の環境構築なし）

1. このフォルダを GitHub リポジトリに push する
2. Actions の「Build FluentBrowser」が自動で走る（手動実行も可）
3. 完了後、Artifacts の `FluentBrowser-win-x64`（zip）をダウンロード
4. 解凍して `FluentBrowser.exe` を実行（インストール不要のポータブルフォルダ形式）

### ローカル（Windows）

必要なもの：

- Visual Studio 2022（ワークロード「C++ によるデスクトップ開発」＋
  「Windows アプリケーション開発」/ Windows App SDK の C++ ツール、Windows 11 SDK 10.0.26100）
- CMake（VS 同梱で可）、NuGet CLI、PowerShell

```powershell
powershell -ExecutionPolicy Bypass -File scripts\fetch-cef.ps1   # CEF 取得 + wrapper ビルド（初回のみ、大きめ）
powershell -ExecutionPolicy Bypass -File scripts\build.ps1       # dist\FluentBrowser\ と zip を生成
```

CEF のバージョンは `scripts/fetch-cef.ps1` 先頭の `$CefVersion` で固定しています。
ダウンロード元は https://cef-builds.spotifycdn.com/index.html#windows64 です。
もし 404 になる場合はそのページで windows64 の現行バージョン文字列を確認し、
`$CefVersion` を差し替えてください（形式：`131.3.5+g<hash>+chromium-131.0.6778.205` のような並び）。
チェックサムはスクリプトに埋め込んでいません。

## v0.1 の既知の制限

- 拡張機能なし、Sync なし、単一ウィンドウのみ、タブの切り離しなし
- CEF 子 HWND のエアスペース制限（上記）。ページ内検索は基本機能のみ（件数表示なし）
- ダウンロードは保存先ダイアログなしで「ダウンロード」フォルダへ直接保存
- エラーページは簡易表示、証明書エラー等の専用 UI なし
- 終了時は全タブを閉じてから CEF を終了する簡易処理（非同期終了の完全な待機は未実装）

## ロードマップ案

- タブのドラッグ切り離し／複数ウィンドウ、セッション復元
- ダウンロード一覧の永続化と進捗 UI の改善、保存先選択
- 検索バーの候補（履歴・お気に入りからのサジェスト）
- CEF サンドボックス有効化の検討、署名付き配布

Created by seasea1010 の表記は Lumo 側の決定事項であり、本プロジェクトの About 表記は未定です（必要なら追加してください）。
