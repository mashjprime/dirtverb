@echo off
echo ===================================
echo  Cinder Plugin Installer
echo ===================================
echo.

set VST3_DIR=C:\Program Files\Common Files\VST3
set PLUGIN_PATH=build\Cinder_artefacts\Release\VST3\Cinder.vst3

if not exist "%PLUGIN_PATH%" (
    echo ERROR: Plugin not found at %PLUGIN_PATH%
    echo Please run build.bat first!
    pause
    exit /b 1
)

echo Installing to: %VST3_DIR%
echo.

:: Need admin rights to copy to Program Files
net session >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo This script requires administrator privileges.
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: Copy plugin
xcopy /E /I /Y "%PLUGIN_PATH%" "%VST3_DIR%\Cinder.vst3"

if %ERRORLEVEL% equ 0 (
    echo.
    echo ===================================
    echo  INSTALLATION SUCCESSFUL!
    echo ===================================
    echo.
    echo Plugin installed to:
    echo   %VST3_DIR%\Cinder.vst3
    echo.
    echo Now rescan plugins in your DAW:
    echo   Ableton: Preferences ^> Plugins ^> Rescan
    echo.
) else (
    echo.
    echo ERROR: Installation failed!
)

pause
