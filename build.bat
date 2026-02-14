@echo off
echo ===================================
echo  Cinder Build Script
echo ===================================
echo.

:: Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake not found!
    echo.
    echo Please install CMake:
    echo   1. Download from https://cmake.org/download/
    echo   2. Or run: winget install Kitware.CMake
    echo.
    pause
    exit /b 1
)

:: Check for Visual Studio
if not exist "%ProgramFiles%\Microsoft Visual Studio\2026" (
    if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2026" (
        echo ERROR: Visual Studio 2026 not found!
        echo.
        echo Please install Visual Studio 2022:
        echo   1. Download from https://visualstudio.microsoft.com/downloads/
        echo   2. Select "Desktop development with C++" workload
        echo.
        pause
        exit /b 1
    )
)

echo Prerequisites found. Configuring build...
echo.

:: Configure
cmake -B build -G "Visual Studio 18 2026" -A x64
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo Configuration complete. Building...
echo.

:: Build
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ===================================
echo  BUILD SUCCESSFUL!
echo ===================================
echo.
echo Plugin location:
echo   build\Cinder_artefacts\Release\VST3\Cinder.vst3
echo.
echo To install, run install.bat
echo.
pause
