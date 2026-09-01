[CmdletBinding()]
param(
    [string]$QtVersion = "6.10.3",
    [string]$VlcVersion = "3.0.23",
    [string]$WixVersion = "4.0.6"
)

$ErrorActionPreference = "Stop"
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$dependenciesDirectory = Join-Path $repositoryRoot ".deps"
$downloadsDirectory = Join-Path $dependenciesDirectory "downloads"
$qtDirectory = Join-Path $dependenciesDirectory "Qt"
$vlcDirectory = Join-Path $dependenciesDirectory "vlc-$VlcVersion"
$wixDirectory = Join-Path $dependenciesDirectory "wix-$WixVersion"
$pythonEnvironment = Join-Path $dependenciesDirectory "bootstrap-python"

New-Item -ItemType Directory -Force -Path $dependenciesDirectory, $downloadsDirectory | Out-Null

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio 2022 with the Desktop development with C++ workload is required."
}
$visualStudio = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$compiler = & $vswhere -latest -products * -find "VC\Tools\MSVC\**\bin\Hostx64\x64\cl.exe" | Select-Object -First 1
if (-not $visualStudio -and -not $compiler) {
    throw "Install the Visual Studio 2022 'Desktop development with C++' workload, then run this script again."
}

$qtInstall = Join-Path $qtDirectory "$QtVersion\msvc2022_64"
if (-not (Test-Path -LiteralPath (Join-Path $qtInstall "bin\qtpaths.exe"))) {
    if (-not (Test-Path -LiteralPath (Join-Path $pythonEnvironment "Scripts\python.exe"))) {
        & py -m venv $pythonEnvironment
    }
    $bootstrapPython = Join-Path $pythonEnvironment "Scripts\python.exe"
    & $bootstrapPython -m pip install --disable-pip-version-check "aqtinstall==3.3.0"
    & $bootstrapPython -m aqt install-qt windows desktop $QtVersion win64_msvc2022_64 `
        --outputdir $qtDirectory `
        --modules qtshadertools
    if ($LASTEXITCODE -ne 0) {
        throw "Qt installation failed."
    }
}

$vlcArchive = Join-Path $downloadsDirectory "vlc-$VlcVersion-win64.zip"
$vlcSourceArchive = Join-Path $downloadsDirectory "vlc-$VlcVersion.tar.xz"
$vlcSourceDirectory = Join-Path $dependenciesDirectory "vlc-source-$VlcVersion"
$expectedVlcSha256 = switch ($VlcVersion) {
    "3.0.23" { "992d19dbd0b8a7cde9167d2f7780b1ef6f92acc8a71acfa736101a21f35181e1" }
    default { throw "No trusted checksum is configured for VLC $VlcVersion." }
}
$expectedVlcSourceSha256 = switch ($VlcVersion) {
    "3.0.23" { "e891cae6aa3ccda69bf94173d5105cbc55c7a7d9b1d21b9b21666e69eff3e7e0" }
    default { throw "No trusted source checksum is configured for VLC $VlcVersion." }
}

if (-not (Test-Path -LiteralPath $vlcDirectory)) {
    if (-not (Test-Path -LiteralPath $vlcArchive)) {
        $vlcUrl = "https://download.videolan.org/pub/videolan/vlc/$VlcVersion/win64/vlc-$VlcVersion-win64.zip"
        Write-Host "Downloading the official LibVLC SDK and runtime..."
        Invoke-WebRequest -Uri $vlcUrl -OutFile $vlcArchive
    }
    $actualHash = (Get-FileHash -LiteralPath $vlcArchive -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualHash -ne $expectedVlcSha256) {
        throw "The VLC archive checksum does not match. Delete '$vlcArchive' before retrying."
    }
    Expand-Archive -LiteralPath $vlcArchive -DestinationPath $dependenciesDirectory -Force
}

$vlcHeader = Join-Path $vlcDirectory "sdk\include\vlc\vlc.h"
if (-not (Test-Path -LiteralPath $vlcHeader)) {
    if (-not (Test-Path -LiteralPath $vlcSourceArchive)) {
        $sourceUrl = "https://download.videolan.org/pub/videolan/vlc/$VlcVersion/vlc-$VlcVersion.tar.xz"
        Write-Host "Downloading matching official LibVLC headers..."
        Invoke-WebRequest -Uri $sourceUrl -OutFile $vlcSourceArchive
    }
    $actualSourceHash = (Get-FileHash -LiteralPath $vlcSourceArchive -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualSourceHash -ne $expectedVlcSourceSha256) {
        throw "The VLC source checksum does not match. Delete '$vlcSourceArchive' before retrying."
    }
    New-Item -ItemType Directory -Force -Path $vlcSourceDirectory | Out-Null
    & tar -xf $vlcSourceArchive -C $vlcSourceDirectory --strip-components=2 "vlc-$VlcVersion/include/vlc"
    if ($LASTEXITCODE -ne 0) { throw "Extracting the LibVLC headers failed." }
    $sdkInclude = Join-Path $vlcDirectory "sdk\include"
    New-Item -ItemType Directory -Force -Path $sdkInclude | Out-Null
    Copy-Item -LiteralPath (Join-Path $vlcSourceDirectory "vlc") -Destination $sdkInclude -Recurse -Force
}

$vlcImportLibrary = Join-Path $vlcDirectory "sdk\lib\libvlc.lib"
if (-not (Test-Path -LiteralPath $vlcImportLibrary)) {
    $compilerDirectory = Split-Path -Parent $compiler
    $dumpbin = Join-Path $compilerDirectory "dumpbin.exe"
    $libraryTool = Join-Path $compilerDirectory "lib.exe"
    if (-not (Test-Path -LiteralPath $dumpbin) -or -not (Test-Path -LiteralPath $libraryTool)) {
        throw "Visual Studio's dumpbin.exe and lib.exe are required to generate the LibVLC import library."
    }
    $sdkLibraryDirectory = Split-Path -Parent $vlcImportLibrary
    New-Item -ItemType Directory -Force -Path $sdkLibraryDirectory | Out-Null
    $definitionFile = Join-Path $sdkLibraryDirectory "libvlc.def"
    $exports = & $dumpbin /nologo /exports (Join-Path $vlcDirectory "libvlc.dll") |
        ForEach-Object {
            if ($_ -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]+\s+(\S+)') { $Matches[1] }
        }
    if (-not $exports) { throw "No exports were found in libvlc.dll." }
    @("LIBRARY libvlc.dll", "EXPORTS") + ($exports | ForEach-Object { "  $_" }) |
        Set-Content -LiteralPath $definitionFile -Encoding Ascii
    & $libraryTool /nologo "/def:$definitionFile" /machine:x64 "/out:$vlcImportLibrary"
    if ($LASTEXITCODE -ne 0) { throw "Generating the LibVLC import library failed." }
}

$vlcCacheGenerator = Join-Path $vlcDirectory "vlc-cache-gen.exe"
$vlcPluginsDirectory = Join-Path $vlcDirectory "plugins"
if (-not (Test-Path -LiteralPath $vlcCacheGenerator)) {
    throw "The VLC plugin cache generator is missing from the runtime."
}
& $vlcCacheGenerator (Resolve-Path -LiteralPath $vlcPluginsDirectory).Path
if ($LASTEXITCODE -ne 0) { throw "Generating the VLC plugin cache failed." }
$vlcPluginCache = Join-Path $vlcPluginsDirectory "plugins.dat"
if (-not (Test-Path -LiteralPath $vlcPluginCache) -or (Get-Item $vlcPluginCache).Length -lt 1024) {
    throw "The generated VLC plugin cache is missing or invalid."
}

$wixExecutable = Join-Path $wixDirectory "tools\net6.0\any\wix.exe"
if (-not (Test-Path -LiteralPath $wixExecutable)) {
    $wixPackage = Join-Path $downloadsDirectory "wix.$WixVersion.nupkg"
    $expectedWixSha256 = switch ($WixVersion) {
        "4.0.6" { "a94dd42ae1fb56b32da180e2173ceda4f0d10b4c8871c5ee59ecb502131a1eb6" }
        default { throw "No trusted checksum is configured for WiX $WixVersion." }
    }
    if (-not (Test-Path -LiteralPath $wixPackage)) {
        Write-Host "Downloading the pinned WiX packaging tool..."
        Invoke-WebRequest `
            -Uri "https://api.nuget.org/v3-flatcontainer/wix/$WixVersion/wix.$WixVersion.nupkg" `
            -OutFile $wixPackage
    }
    $actualWixHash = (Get-FileHash -LiteralPath $wixPackage -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualWixHash -ne $expectedWixSha256) {
        throw "The WiX package checksum does not match. Delete '$wixPackage' before retrying."
    }
    New-Item -ItemType Directory -Force -Path $wixDirectory | Out-Null
    & tar -xf $wixPackage -C $wixDirectory
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $wixExecutable)) {
        throw "Extracting the WiX packaging tool failed."
    }
}

$installedWixExtensions = & $wixExecutable extension list -g
if ($LASTEXITCODE -ne 0) { throw "Reading the WiX extension cache failed." }
if ($installedWixExtensions -notcontains "WixToolset.UI.wixext $WixVersion") {
    & $wixExecutable extension add -g "WixToolset.UI.wixext/$WixVersion"
    if ($LASTEXITCODE -ne 0) { throw "Installing the WiX UI extension failed." }
}

Write-Host "Dependencies are ready."
Write-Host "Qt:     $qtInstall"
Write-Host "LibVLC: $vlcDirectory"
Write-Host "WiX:    $wixDirectory"
