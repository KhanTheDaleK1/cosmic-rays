@echo off
setlocal

:: Run version generation script to ensure it's up to date
python Scripts/generate_version.py

:: Read version from version.txt
set /p VERSION=<version.txt

echo Cosmic Rays - 1-Click Installer for Windows (%VERSION%)
echo -----------------------------------------------------------

:: --- Binary Installation Path ---
set BINARY_FOUND=0

:: Search for the most recent release folder (Release_*)
for /f "delims=" %%i in ('dir /b /ad /o-n releases\Release_* 2^>nul') do (
    set LATEST_RELEASE=releases\%%i
    goto :found_release
)

:found_release
if defined LATEST_RELEASE (
    echo Found release folder: %LATEST_RELEASE%
    
    set VST3_PATH=%COMMONPROGRAMFILES%\VST3
    set LV2_PATH=%APPDATA%\LV2

    if exist "%LATEST_RELEASE%\Windows\Cosmic Rays.vst3" (
        echo Found pre-built VST3. Installing...
        if not exist "%VST3_PATH%" mkdir "%VST3_PATH%"
        xcopy /E /I /Y "%LATEST_RELEASE%\Windows\Cosmic Rays.vst3" "%VST3_PATH%\Cosmic Rays.vst3"
        set BINARY_FOUND=1
    fi

    if exist "%LATEST_RELEASE%\Windows\Cosmic Rays.lv2" (
        echo Found pre-built LV2. Installing...
        if not exist "%LV2_PATH%" mkdir "%LV2_PATH%"
        xcopy /E /I /Y "%LATEST_RELEASE%\Windows\Cosmic Rays.lv2" "%LV2_PATH%\Cosmic Rays.lv2"
        set BINARY_FOUND=1
    fi
)

if %BINARY_FOUND% equ 1 (
    echo.
    echo Cosmic Rays binary installation complete!
    echo You may need to restart your DAW to see the plugin.
    pause
    exit /b 0
)

:: --- Source Build Fallback ---
echo Pre-built binaries not found in releases\. Attempting to build from source...

:: Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: Pre-built binaries not found and CMake is not installed.
    echo Please install CMake to build from source, or download the full release package.
    pause
    exit /b 1
)

:: Build the project
echo Building Cosmic Rays...
if not exist build mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo Error: Build failed.
    pause
    exit /b 1
)

:: Installation paths from build
set VST3_PATH=%COMMONPROGRAMFILES%\VST3
if not exist "%VST3_PATH%" mkdir "%VST3_PATH%"

echo Installing VST3 to %VST3_PATH%...
xcopy /E /I /Y "Source\CosmicRays_artefacts\Release\VST3\Cosmic Rays.vst3" "%VST3_PATH%\Cosmic Rays.vst3"

echo.
echo Cosmic Rays build and installation complete!
echo You may need to restart your DAW to see the plugin.
pause

