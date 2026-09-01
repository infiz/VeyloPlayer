@echo off
setlocal

set "SCRIPT_DIRECTORY=%~dp0"

echo Building the VeyloPlayer Windows release package...
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIRECTORY%build-windows.ps1" -Configuration Release -Package -Bootstrap %*
set "BUILD_EXIT_CODE=%ERRORLEVEL%"

if not "%BUILD_EXIT_CODE%"=="0" (
    echo.
    echo VeyloPlayer packaging failed with exit code %BUILD_EXIT_CODE%.
    exit /b %BUILD_EXIT_CODE%
)

echo.
echo VeyloPlayer packages are available in the dist directory.
exit /b 0
