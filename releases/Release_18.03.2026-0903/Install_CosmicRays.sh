#!/bin/bash

# Cosmic Rays - 1-Click Installer for macOS and Linux
# Installs pre-built binaries or builds from source as a fallback.

set -e

# Read the current version
if [ -f version.txt ]; then
    VERSION=$(cat version.txt)
else
    VERSION="Unknown"
fi

echo "=== Cosmic Rays Installer ($VERSION) ==="
OS="$(uname)"

# --- Binary Installation Path ---
BINARY_FOUND=false

if [ "$OS" == "Darwin" ]; then
    echo "Detected macOS."
    AU_PATH="$HOME/Library/Audio/Plug-Ins/Components"
    VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3"
    LV2_PATH="$HOME/Library/Audio/Plug-Ins/LV2"
    
    # 1. Try pre-built binaries
    if [ -d "macOS/Cosmic Rays.component" ] || [ -d "macOS/Cosmic Rays.vst3" ]; then
        echo "Found pre-built binaries. Installing..."
        mkdir -p "$AU_PATH" "$VST3_PATH" "$LV2_PATH"
        [ -d "macOS/Cosmic Rays.component" ] && cp -R "macOS/Cosmic Rays.component" "$AU_PATH/"
        [ -d "macOS/Cosmic Rays.vst3" ] && cp -R "macOS/Cosmic Rays.vst3" "$VST3_PATH/"
        [ -d "macOS/Cosmic Rays.lv2" ] && cp -R "macOS/Cosmic Rays.lv2" "$LV2_PATH/"
        BINARY_FOUND=true
    fi

elif [ "$OS" == "Linux" ]; then
    echo "Detected Linux."
    VST3_PATH="$HOME/.vst3"
    LV2_PATH="$HOME/.lv2"
    
    # 1. Try pre-built binaries
    if [ -d "Linux/Cosmic Rays.vst3" ] || [ -d "Linux/Cosmic Rays.lv2" ]; then
        echo "Found pre-built binaries. Installing..."
        mkdir -p "$VST3_PATH" "$LV2_PATH"
        [ -d "Linux/Cosmic Rays.vst3" ] && cp -R "Linux/Cosmic Rays.vst3" "$VST3_PATH/"
        [ -d "Linux/Cosmic Rays.lv2" ] && cp -R "Linux/Cosmic Rays.lv2" "$LV2_PATH/"
        BINARY_FOUND=true
    fi
fi

if [ "$BINARY_FOUND" = true ]; then
    echo "Cosmic Rays binary installation complete!"
    echo "You may need to restart your DAW to see the plugin."
    exit 0
fi

# --- Source Build Fallback ---
echo "Pre-built binaries not found for your platform."
read -p "Attempt to build from source? (y/n): " confirm
if [[ $confirm != [yY] ]]; then
    echo "Installation cancelled."
    exit 1
fi

case "$OS" in
    Linux)
        # Install dependencies (requires sudo)
        if [ -f /etc/debian_version ]; then
            echo "Installing dependencies via apt..."
            sudo apt update && sudo apt install -y cmake g++ libasound2-dev libjack-jackd2-dev \
                libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev \
                libfreetype6-dev libcurl4-openssl-dev libwebkit2gtk-4.1-dev
        fi
        ;;
    Darwin)
        # Install dependencies (requires Homebrew)
        if command -v brew >/dev/null 2>&1; then
            echo "Installing dependencies via Homebrew..."
            brew install cmake
        fi
        ;;
esac

# Build the project
echo "Building Cosmic Rays..."
cd ../..
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Installation paths from build
if [ "$OS" == "Linux" ]; then
    VST3_PATH="$HOME/.vst3"
    LV2_PATH="$HOME/.lv2"
    mkdir -p "$VST3_PATH" "$LV2_PATH"
    cp -r Source/CosmicRays_artefacts/Release/VST3/Cosmic\ Rays.vst3 "$VST3_PATH/"
    cp -r Source/CosmicRays_artefacts/Release/LV2/Cosmic\ Rays.lv2 "$LV2_PATH/"
elif [ "$OS" == "Darwin" ]; then
    AU_PATH="$HOME/Library/Audio/Plug-Ins/Components"
    VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3"
    LV2_PATH="$HOME/Library/Audio/Plug-Ins/LV2"
    mkdir -p "$AU_PATH" "$VST3_PATH" "$LV2_PATH"
    cp -r Source/CosmicRays_artefacts/Release/AU/Cosmic\ Rays.component "$AU_PATH/"
    cp -r Source/CosmicRays_artefacts/Release/VST3/Cosmic\ Rays.vst3 "$VST3_PATH/"
    [ -d "Source/CosmicRays_artefacts/Release/LV2/Cosmic Rays.lv2" ] && cp -r Source/CosmicRays_artefacts/Release/LV2/Cosmic\ Rays.lv2 "$LV2_PATH/"
fi

echo "Cosmic Rays build and installation complete!"


