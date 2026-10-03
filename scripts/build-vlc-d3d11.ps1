[CmdletBinding()]
param(
    [string]$VlcVersion = "3.0.24",
    [string]$WslDistribution = "Ubuntu-24.04"
)

$ErrorActionPreference = "Stop"
if ($VlcVersion -ne "3.0.24") { throw "The Direct3D 11 patch requires VLC 3.0.24." }
$repositoryRoot = Split-Path -Parent $PSScriptRoot
$recipe = Join-Path $PSScriptRoot "vlc-d3d11"
$archive = Join-Path $repositoryRoot ".deps\downloads\vlc-$VlcVersion.tar.xz"
$output = Join-Path $repositoryRoot ".deps\vlc-d3d11-$VlcVersion"
$runtime = Join-Path $repositoryRoot ".deps\vlc-$VlcVersion"
$manifestPath = Join-Path $output "veylo-d3d11-build.json"
$plugin = Join-Path $output "libdirect3d11_plugin.dll"
$expectedSourceHash = "e7cab503d1d7d5849b89d2cf0e1ee60d0ef6d012407791b644b9cfc0cc225fdf"

function Get-Sha256([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

if (-not (Test-Path -LiteralPath $archive)) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $archive) | Out-Null
    Invoke-WebRequest -Uri "https://download.videolan.org/pub/videolan/vlc/$VlcVersion/vlc-$VlcVersion.tar.xz" -OutFile $archive
}
if ((Get-Sha256 $archive) -ne $expectedSourceHash) { throw "VLC source checksum mismatch." }

$cached = $false
if ((Test-Path -LiteralPath $manifestPath) -and (Test-Path -LiteralPath $plugin)) {
    try {
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        $cached = $manifest.source_sha256 -eq $expectedSourceHash -and
            $manifest.plugin_sha256 -eq (Get-Sha256 $plugin)
        foreach ($name in @("build.py", "config.h", "planar-422.patch", "plugin.rc")) {
            $cached = $cached -and $manifest.inputs.$name -eq (Get-Sha256 (Join-Path $recipe $name))
        }
        foreach ($name in @("gcc-mingw-w64-base", "mingw-w64-common")) {
            $cached = $cached -and (Test-Path -LiteralPath (Join-Path $output "$name-copyright.txt"))
        }
    } catch { $cached = $false }
}

if (-not $cached) {
    New-Item -ItemType Directory -Force -Path $output | Out-Null
    function Convert-ToWslPath([string]$Path) {
        $converted = & wsl -d $WslDistribution --exec wslpath -a -u $Path
        if ($LASTEXITCODE -ne 0) {
            throw "WSL $WslDistribution is required to build the patched VLC output. See README.md."
        }
        return "$converted".Trim()
    }
    $builderPath = Convert-ToWslPath (Join-Path $recipe "build.py")
    $archivePath = Convert-ToWslPath $archive
    $outputPath = Convert-ToWslPath $output
    & wsl -d $WslDistribution --exec python3 $builderPath --archive $archivePath --output $outputPath
    if ($LASTEXITCODE -ne 0) { throw "Building the patched Direct3D 11 plugin failed." }
}

# Only modify the private development runtime after a successful verified build.
$manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if ((Get-Sha256 $plugin) -ne $manifest.plugin_sha256) { throw "Patched plugin checksum mismatch." }
$fileVersion = (Get-Item -LiteralPath $plugin).VersionInfo.FileVersion
if (-not $fileVersion -or [version]$fileVersion -le [version]"3.0.24.0") {
    throw "The patched plugin must have a Windows file version newer than stock VLC for MSI upgrades."
}
function Copy-IfChanged([string]$Source, [string]$Destination) {
    if (-not (Test-Path -LiteralPath $Destination) -or
        (Get-Sha256 $Source) -ne (Get-Sha256 $Destination)) {
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
    }
}
Copy-IfChanged $plugin (Join-Path $runtime "plugins\video_output\libdirect3d11_plugin.dll")
Copy-IfChanged $manifestPath (Join-Path $runtime "veylo-d3d11-build.json")
foreach ($name in @("gcc-mingw-w64-base", "mingw-w64-common")) {
    Copy-IfChanged (Join-Path $output "$name-copyright.txt") (Join-Path $runtime "$name-copyright.txt")
}
