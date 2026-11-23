# Builds FluentBrowser (Release x64) and assembles dist\FluentBrowser\ portable folder.
# Prereq: scripts\fetch-cef.ps1 has been run once.
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Cef = Join-Path $Root "third_party\cef\current"
if (-not (Test-Path $Cef)) { throw "CEF not found. Run scripts\fetch-cef.ps1 first." }
# NuGet restore (nuget.exe on PATH, or VS's bundled one)
$nuget = (Get-Command nuget -ErrorAction SilentlyContinue).Source
if (-not $nuget) { throw "nuget.exe not found on PATH. Install NuGet CLI or use Visual Studio Developer PowerShell." }
& $nuget restore (Join-Path $Root "FluentBrowser.sln")
# Find msbuild via vswhere
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
if (-not $msbuild) { throw "MSBuild not found (Visual Studio 2022 required)." }
& $msbuild (Join-Path $Root "FluentBrowser.sln") /p:Configuration=Release /p:Platform=x64 /m
if ($LASTEXITCODE -ne 0) { throw "MSBuild failed: $LASTEXITCODE" }
$Out = Join-Path $Root "x64\Release\FluentBrowser"
$Dist = Join-Path $Root "dist\FluentBrowser"
if (Test-Path $Dist) { Remove-Item $Dist -Recurse -Force }
New-Item -ItemType Directory -Force $Dist | Out-Null
Copy-Item "$Out\*" $Dist -Recurse -Force
# CEF runtime (Release)
$CefRel = Join-Path $Cef "Release"
foreach ($f in @("libcef.dll","chrome_elf.dll","d3dcompiler_47.dll","dxcompiler.dll","dxil.dll","icudtl.dat","resources.pak","chrome_100_percent.pak","chrome_200_percent.pak","snapshot_blob.bin","v8_context_snapshot.bin","libEGL.dll","libGLESv2.dll","vk_swiftshader.dll","vk_swiftshader_icd.json","vulkan-1.dll")) {
    $p = Join-Path $CefRel $f; if (Test-Path $p) { Copy-Item $p $Dist -Force }
}
if (Test-Path (Join-Path $CefRel "locales")) { Copy-Item (Join-Path $CefRel "locales") $Dist -Recurse -Force }
if (Test-Path (Join-Path $Cef "Resources\locales")) { Copy-Item (Join-Path $Cef "Resources\locales") $Dist -Recurse -Force }
Write-Host "Portable folder ready: $Dist"
Compress-Archive -Path "$Dist\*" -DestinationPath (Join-Path $Root "dist\FluentBrowser-win-x64.zip") -Force
Write-Host "Zip: dist\FluentBrowser-win-x64.zip"
