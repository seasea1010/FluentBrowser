# Downloads the CEF binary distribution and builds libcef_dll_wrapper (Release+Debug).
# Usage: powershell -ExecutionPolicy Bypass -File scripts\fetch-cef.ps1
$ErrorActionPreference = "Stop"
# === CEF version (edit here if needed) ===
# Verify the exact string on https://cef-builds.spotifycdn.com/index.html#windows64
# Pattern: cef_binary_<CEF_VERSION>_windows64.tar.bz2  ('+' is URL-encoded as %2B)
$CefVersion = "131.3.5+gdda98e2+chromium-131.0.6778.205"
$Root = Split-Path -Parent $PSScriptRoot
$Dest = Join-Path $Root "third_party\cef"
New-Item -ItemType Directory -Force $Dest | Out-Null
$Archive = Join-Path $Dest "cef_binary_$($CefVersion)_windows64.tar.bz2"
$Encoded = $CefVersion -replace '\+', '%2B'
$Url = "https://cef-builds.spotifycdn.com/cef_binary_$($Encoded)_windows64.tar.bz2"
$Extracted = Join-Path $Dest "cef_binary_$($CefVersion)_windows64"
if (-not (Test-Path $Extracted)) {
    if (-not (Test-Path $Archive)) {
        Write-Host "Downloading $Url"
        Invoke-WebRequest -Uri $Url -OutFile $Archive
    }
    Write-Host "Extracting (tar.bz2 via Windows tar)..."
    tar -xjf $Archive -C $Dest
}
if (-not (Test-Path $Extracted)) { throw "CEF extraction failed: $Extracted not found. Check `$CefVersion against the index page." }
# Stable alias used by Cef.props
$Current = Join-Path $Dest "current"
if (Test-Path $Current) { Remove-Item $Current -Recurse -Force }
# Junction avoids copying ~1GB
cmd /c mklink /J "`"$Current`"" "`"$Extracted`"" | Out-Null
# Build libcef_dll_wrapper with CMake (CEF ships CMakeLists.txt at its root)
foreach ($Cfg in @("Release","Debug")) {
    $BuildDir = Join-Path $Extracted "build"
    cmake -S $Extracted -B $BuildDir -G "Visual Studio 17 2022" -A x64
    cmake --build $BuildDir --config $Cfg --target libcef_dll_wrapper
}
Write-Host "CEF ready: $Current"
