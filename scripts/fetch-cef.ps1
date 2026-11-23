# Downloads the CEF binary distribution and builds libcef_dll_wrapper (Release+Debug).
# Usage: powershell -ExecutionPolicy Bypass -File scripts\fetch-cef.ps1
$ErrorActionPreference = "Stop"
# === CEF version (edit here if needed) ===
# Verify the exact string on https://cef-builds.spotifycdn.com/index.html#windows64
# Pattern: cef_binary_<CEF_VERSION>_windows64.tar.bz2  ('+' is URL-encoded as %2B)
$CefVersion = "131.3.5+g573cec5+chromium-131.0.6778.205"
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
        try {
            Invoke-WebRequest -Uri $Url -OutFile $Archive
        } catch {
            # Fallback: resolve the exact build for this Chromium version from the CEF index
            Write-Host "Pinned build not found, resolving from CEF index..."
            $ChromiumVer = ($CefVersion -split 'chromium-')[1]
            $Index = Invoke-RestMethod -Uri "https://cef-builds.spotifycdn.com/index.json"
            $Entry = $Index.windows64.versions | Where-Object { $_.chromium_version -eq $ChromiumVer -and $_.channel -eq "stable" } | Select-Object -First 1
            if (-not $Entry) { throw "No windows64 CEF build found in index for Chromium $ChromiumVer" }
            $script:CefVersion = $Entry.cef_version
            $Archive = Join-Path $Dest "cef_binary_$($CefVersion)_windows64.tar.bz2"
            $Encoded = $CefVersion -replace '\+', '%2B'
            $Url = "https://cef-builds.spotifycdn.com/cef_binary_$($Encoded)_windows64.tar.bz2"
            $script:Extracted = Join-Path $Dest "cef_binary_$($CefVersion)_windows64"
            Write-Host "Resolved $CefVersion, downloading $Url"
            Invoke-WebRequest -Uri $Url -OutFile $Archive
        }
    }
    # Extract with 7-Zip when available: Windows bsdtar hung silently for ~1h on this
    # archive in GitHub Actions run #2 (2026-11-23). Fall back to tar otherwise.
    $SevenZip = $null
    if (Test-Path "C:\Program Files\7-Zip\7z.exe") { $SevenZip = "C:\Program Files\7-Zip\7z.exe" }
    elseif (Get-Command 7z -ErrorAction SilentlyContinue) { $SevenZip = "7z" }
    if ($SevenZip) {
        Write-Host "Extracting (tar.bz2 via 7-Zip)..."
        $TarFile = $Archive -replace '\.bz2$', ''
        & $SevenZip x $Archive "-o$Dest" -y | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "7-Zip bzip2 extraction failed: $LASTEXITCODE" }
        & $SevenZip x $TarFile "-o$Dest" -y | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "7-Zip tar extraction failed: $LASTEXITCODE" }
        Remove-Item $TarFile -Force -ErrorAction SilentlyContinue
    } else {
        Write-Host "Extracting (tar.bz2 via Windows tar)..."
        tar -xjf $Archive -C $Dest
    }
    Write-Host "Extraction done."
}
if (-not (Test-Path $Extracted)) { throw "CEF extraction failed: $Extracted not found. Check `$CefVersion against the index page." }
# Stable alias used by Cef.props
$Current = Join-Path $Dest "current"
if (Test-Path $Current) { Remove-Item $Current -Recurse -Force }
# Junction avoids copying ~1GB
cmd /c mklink /J "`"$Current`"" "`"$Extracted`"" | Out-Null
# Build libcef_dll_wrapper with CMake (CEF ships CMakeLists.txt at its root)
# Release only: CI builds the app in Release; Debug wrapper doubled the time for nothing.
# -DCEF_RUNTIME_LIBRARY_FLAG=/MD: the app builds with /MD (WinUI needs the dynamic
# CRT) but CEF's CMake defaults the wrapper to /MT, which failed run #8 at link time
# with 142x LNK2038 (MT_StaticRelease vs MD_DynamicRelease).
foreach ($Cfg in @("Release")) {
    $BuildDir = Join-Path $Extracted "build"
    cmake -S $Extracted -B $BuildDir -G "Visual Studio 17 2022" -A x64 -DCEF_RUNTIME_LIBRARY_FLAG=/MD
    cmake --build $BuildDir --config $Cfg --target libcef_dll_wrapper
}
Write-Host "CEF ready: $Current"
