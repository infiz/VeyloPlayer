[CmdletBinding()]
param(
    [string]$ApplicationPath,
    [switch]$Unregister
)

$ErrorActionPreference = "Stop"
$classesRoot = "HKCU:\Software\Classes"
$applicationRoot = "HKCU:\Software\VeyloPlayer"
$registeredApplications = "HKCU:\Software\RegisteredApplications"
$openWithApplication = "HKCU:\Software\Classes\Applications\VeyloPlayer.exe"
$appPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\App Paths\VeyloPlayer.exe"
$progId = "VeyloPlayer.Media"
$extensions = @(
    ".mp3", ".m4a", ".aac", ".wav", ".flac", ".ogg",
    ".mp4", ".m4v", ".mov", ".mkv", ".webm", ".avi",
    ".jpg", ".jpeg"
)

if ($Unregister) {
    Remove-Item -LiteralPath (Join-Path $classesRoot $progId) -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $applicationRoot -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $openWithApplication -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $appPath -Recurse -Force -ErrorAction SilentlyContinue
    Remove-ItemProperty -LiteralPath $registeredApplications -Name "VeyloPlayer" -ErrorAction SilentlyContinue
    foreach ($extension in $extensions) {
        Remove-ItemProperty -LiteralPath (Join-Path $classesRoot "$extension\OpenWithProgids") `
            -Name $progId -ErrorAction SilentlyContinue
    }
    Write-Host "VeyloPlayer file-type registrations were removed for the current user."
    exit 0
}

if (-not $ApplicationPath) {
    throw "Provide -ApplicationPath with the absolute path to VeyloPlayer.exe."
}
$resolvedApplication = (Resolve-Path -LiteralPath $ApplicationPath).Path
if ([System.IO.Path]::GetFileName($resolvedApplication) -ne "VeyloPlayer.exe") {
    throw "ApplicationPath must point to VeyloPlayer.exe."
}

New-Item -Path (Join-Path $classesRoot "$progId\shell\open\command") -Force | Out-Null
Set-Item -Path (Join-Path $classesRoot $progId) -Value "VeyloPlayer media file"
Set-Item -Path (Join-Path $classesRoot "$progId\shell\open\command") `
    -Value ('"{0}" "%1"' -f $resolvedApplication)
New-Item -Path (Join-Path $openWithApplication "shell\open\command") -Force | Out-Null
New-Item -Path (Join-Path $openWithApplication "SupportedTypes") -Force | Out-Null
Set-Item -Path (Join-Path $openWithApplication "shell\open\command") `
    -Value ('"{0}" "%1"' -f $resolvedApplication)
New-ItemProperty -Path $openWithApplication -Name "FriendlyAppName" `
    -Value "VeyloPlayer" -PropertyType String -Force | Out-Null
New-ItemProperty -Path $openWithApplication -Name "ApplicationIcon" `
    -Value ('"{0}",0' -f $resolvedApplication) -PropertyType String -Force | Out-Null
New-Item -Path $appPath -Force | Out-Null
Set-Item -Path $appPath -Value $resolvedApplication

$capabilities = Join-Path $applicationRoot "Capabilities"
New-Item -Path (Join-Path $capabilities "FileAssociations") -Force | Out-Null
New-ItemProperty -Path $capabilities -Name "ApplicationName" -Value "VeyloPlayer" -Force | Out-Null
New-ItemProperty -Path $capabilities -Name "ApplicationDescription" `
    -Value "A modern open-source media player" -Force | Out-Null

foreach ($extension in $extensions) {
    New-Item -Path (Join-Path $classesRoot "$extension\OpenWithProgids") -Force | Out-Null
    New-ItemProperty -Path (Join-Path $classesRoot "$extension\OpenWithProgids") `
        -Name $progId -Value "" -PropertyType String -Force | Out-Null
    New-ItemProperty -Path (Join-Path $capabilities "FileAssociations") `
        -Name $extension -Value $progId -PropertyType String -Force | Out-Null
    New-ItemProperty -Path (Join-Path $openWithApplication "SupportedTypes") `
        -Name $extension -Value "" -PropertyType String -Force | Out-Null
}

New-Item -Path $registeredApplications -Force | Out-Null
New-ItemProperty -Path $registeredApplications -Name "VeyloPlayer" `
    -Value "Software\VeyloPlayer\Capabilities" -PropertyType String -Force | Out-Null

if (-not ("VeyloShellAssociation" -as [type])) {
    Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class VeyloShellAssociation {
    [DllImport("shell32.dll")]
    public static extern void SHChangeNotify(uint eventId, uint flags, IntPtr item1, IntPtr item2);
}
"@
}
[VeyloShellAssociation]::SHChangeNotify(0x08000000, 0, [IntPtr]::Zero, [IntPtr]::Zero)

Write-Host "VeyloPlayer is registered as an available player for supported files."
Write-Host "Windows still requires the user to choose it in Default apps."
