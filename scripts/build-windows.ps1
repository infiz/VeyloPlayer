[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [switch]$Bootstrap,
    [switch]$Package,
    [string]$QtVersion = "6.10.3",
    [string]$VlcVersion = "3.0.23",
    [string]$WixVersion = "4.0.6"
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $repositoryRoot "build\windows"
$qtRoot = Join-Path $repositoryRoot ".deps\Qt\$QtVersion\msvc2022_64"
$vlcRoot = Join-Path $repositoryRoot ".deps\vlc-$VlcVersion"
$wixRoot = Join-Path $repositoryRoot ".deps\wix-$WixVersion"

if ($Bootstrap) {
    & (Join-Path $PSScriptRoot "bootstrap-windows.ps1") `
        -QtVersion $QtVersion -VlcVersion $VlcVersion -WixVersion $WixVersion
}

if (-not (Test-Path -LiteralPath (Join-Path $qtRoot "bin\qtpaths.exe"))) {
    throw "Qt was not found. Run scripts/bootstrap-windows.ps1 first, or use -Bootstrap."
}
if (-not (Test-Path -LiteralPath (Join-Path $vlcRoot "sdk\include\vlc\vlc.h"))) {
    throw "The LibVLC SDK was not found. Run scripts/bootstrap-windows.ps1 first, or use -Bootstrap."
}

$vlcCacheGenerator = Join-Path $vlcRoot "vlc-cache-gen.exe"
$vlcPluginsDirectory = Join-Path $vlcRoot "plugins"
if (-not (Test-Path -LiteralPath $vlcCacheGenerator)) {
    throw "The VLC plugin cache generator was not found. Run scripts/bootstrap-windows.ps1 first."
}
& $vlcCacheGenerator (Resolve-Path -LiteralPath $vlcPluginsDirectory).Path
if ($LASTEXITCODE -ne 0) { throw "Generating the VLC plugin cache failed." }
$vlcPluginCache = Join-Path $vlcPluginsDirectory "plugins.dat"
if (-not (Test-Path -LiteralPath $vlcPluginCache) -or (Get-Item $vlcPluginCache).Length -lt 1024) {
    throw "The generated VLC plugin cache is missing or invalid."
}

$env:PATH = "$(Join-Path $qtRoot 'bin');$env:PATH"

New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

& cmake -S $repositoryRoot -B $buildDirectory `
    -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_PREFIX_PATH=$qtRoot" `
    "-DLIBVLC_ROOT=$vlcRoot" `
    -DVEYLO_BUILD_TESTS=ON
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed." }

& cmake --build $buildDirectory --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw "The build failed." }

$applicationPath = Join-Path $buildDirectory "$Configuration\VeyloPlayer.exe"
$deployArguments = @(
    "--qmldir", (Join-Path $repositoryRoot "qml"),
    "--no-translations",
    "--$($Configuration.ToLowerInvariant())",
    $applicationPath
)
& (Join-Path $qtRoot "bin\windeployqt.exe") @deployArguments
if ($LASTEXITCODE -ne 0) { throw "Qt runtime deployment failed." }

& ctest --test-dir $buildDirectory -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed." }

if ($Package) {
    if ($Configuration -ne "Release") {
        throw "Installer packages must be built with -Configuration Release."
    }
    $wixExecutable = Join-Path $wixRoot "tools\net6.0\any\wix.exe"
    if (-not (Test-Path -LiteralPath $wixExecutable)) {
        throw "WiX was not found. Run scripts/bootstrap-windows.ps1 or use -Bootstrap."
    }
    $env:PATH = "$(Split-Path -Parent $wixExecutable);$env:PATH"
    New-Item -ItemType Directory -Force -Path (Join-Path $repositoryRoot "dist") | Out-Null
    Push-Location (Join-Path $repositoryRoot "dist")
    try {
        & cpack --config (Join-Path $buildDirectory "CPackConfig.cmake") -C Release -G ZIP
        if ($LASTEXITCODE -ne 0) { throw "ZIP packaging failed." }
        & cpack --config (Join-Path $buildDirectory "CPackConfig.cmake") -C Release -G WIX
        if ($LASTEXITCODE -ne 0) { throw "MSI packaging failed." }
    } finally {
        Pop-Location
    }
}

Write-Host "Build complete: $(Join-Path $buildDirectory $Configuration)"
