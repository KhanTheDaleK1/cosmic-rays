@echo off
setlocal

:: Run version generation script to ensure it's up to date
python generate_version.py

:: Read version from version.txt
set /p VERSION=<version.txt

echo Cosmic Rays - 1-Click Installer for Windows (%VERSION%)
echo -----------------------------------------------------------

:: Check for CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake not found. Please install CMake and try again.
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

:: Installation paths
set VST3_PATH=%COMMONPROGRAMFILES%\VST3
if not exist "%VST3_PATH%" mkdir "%VST3_PATH%"

echo Installing VST3 to %VST3_PATH%...
xcopy /E /I /Y "CosmicRays_artefacts\Release\VST3\Cosmic Rays.vst3" "%VST3_PATH%\Cosmic Rays.vst3"

echo.
echo Cosmic Rays installation complete!
echo You may need to restart your DAW to see the plugin.
pause
