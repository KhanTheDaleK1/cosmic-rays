@echo off
setlocal

:: Read version from version.txt if it exists
if exist "version.txt" (
    set /p VERSION=<"version.txt"
) else (
    set VERSION=Unknown
)

echo Cosmic Rays - 1-Click Installer for Windows (%VERSION%)
echo -----------------------------------------------------------

:: Define installation paths
set VST3_PATH=%COMMONPROGRAMFILES%\VST3
set LV2_PATH=%APPDATA%\LV2

:: 1. Try to install pre-built binaries
echo Checking for pre-built binaries...

set BINARY_FOUND=0

if exist "Windows\Cosmic Rays.vst3" (
    echo Found pre-built VST3. Installing...
    if not exist "%VST3_PATH%" mkdir "%VST3_PATH%"
    xcopy /E /I /Y "Windows\Cosmic Rays.vst3" "%VST3_PATH%\Cosmic Rays.vst3"
    set BINARY_FOUND=1
)

if exist "Windows\Cosmic Rays.lv2" (
    echo Found pre-built LV2. Installing...
    if not exist "%LV2_PATH%" mkdir "%LV2_PATH%"
    xcopy /E /I /Y "Windows\Cosmic Rays.lv2" "%LV2_PATH%\Cosmic Rays.lv2"
    set BINARY_FOUND=1
)

if %BINARY_FOUND% equ 1 (
    echo.
    echo Cosmic Rays binary installation complete!
    echo You may need to restart your DAW to see the plugin.
    pause
    exit /b 0
)

:: 2. Fallback to building from source (only if binaries were not found)
echo Pre-built binaries not found in the package.
echo If you have CMake and a C++ compiler installed, you can attempt to build from source.

set /p BUILD_FROM_SOURCE="Build from source? (y/n): "
if /i "%BUILD_FROM_SOURCE%" neq "y" (
    echo.
    echo Installation cancelled. Please visit beechem.site/cosmic-rays to download the full package.
    pause
    exit /b 1
)

:: Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake not found.
    echo Please install CMake to build from source, or download the full release package.
    pause
    exit /b 1
)

:: Build the project (Assuming we are in releases/Release_...)
echo Building Cosmic Rays...
cd ..\..
if not exist build mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Error: Build failed.
    pause
    exit /b 1
)

:: Install from build artifacts
echo Installing built VST3 to %VST3_PATH%...
if not exist "%VST3_PATH%" mkdir "%VST3_PATH%"
xcopy /E /I /Y "Source\CosmicRays_artefacts\Release\VST3\Cosmic Rays.vst3" "%VST3_PATH%\Cosmic Rays.vst3"

echo Installing built LV2 to %LV2_PATH%...
if not exist "%LV2_PATH%" mkdir "%LV2_PATH%"
xcopy /E /I /Y "Source\CosmicRays_artefacts\Release\LV2\Cosmic Rays.lv2" "%LV2_PATH%\Cosmic Rays.lv2"

echo.
echo Cosmic Rays build and installation complete!
echo You may need to restart your DAW to see the plugin.
pause


