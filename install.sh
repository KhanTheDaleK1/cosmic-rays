#!/bin/bash

# Cosmic Rays - 1-Click Installer for macOS and Linux
# Installs dependencies, builds the project, and copies the plugin files.

set -e

# Detect OS
OS="$(uname)"
case "$OS" in
    Linux)
        echo "Detected Linux."
        # Install dependencies (requires sudo)
        if [ -f /etc/debian_version ]; then
            echo "Installing dependencies via apt..."
            sudo apt update && sudo apt install -y cmake g++ libasound2-dev libjack-jackd2-dev 
                libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev 
                libfreetype6-dev libcurl4-openssl-dev libwebkit2gtk-4.1-dev
        else
            echo "Non-Debian based Linux detected. Please ensure you have the necessary JUCE dependencies installed."
        fi
        ;;
    Darwin)
        echo "Detected macOS."
        # Install dependencies (requires Homebrew)
        if command -v brew >/dev/null 2>&1; then
            echo "Installing dependencies via Homebrew..."
            brew install cmake
        else
            echo "Homebrew not found. Please install Homebrew or manually install CMake."
        fi
        ;;
    *)
        echo "Unsupported OS: $OS"
        exit 1
        ;;
esac

# Build the project
echo "Building Cosmic Rays..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Installation paths
if [ "$OS" == "Linux" ]; then
    VST3_PATH="$HOME/.vst3"
    LV2_PATH="$HOME/.lv2"
    mkdir -p "$VST3_PATH" "$LV2_PATH"
    
    echo "Installing VST3 to $VST3_PATH..."
    cp -r CosmicRays_artefacts/Release/VST3/Cosmic\ Rays.vst3 "$VST3_PATH/"
    
    echo "Installing LV2 to $LV2_PATH..."
    cp -r CosmicRays_artefacts/Release/LV2/Cosmic\ Rays.lv2 "$LV2_PATH/"
    
    echo "Cosmic Rays installation complete!"
    echo "You may need to restart your DAW (e.g., Zrythm, Bitwig) to see the plugin."

elif [ "$OS" == "Darwin" ]; then
    AU_PATH="$HOME/Library/Audio/Plug-Ins/Components"
    VST3_PATH="$HOME/Library/Audio/Plug-Ins/VST3"
    mkdir -p "$AU_PATH" "$VST3_PATH"
    
    echo "Installing AU to $AU_PATH..."
    cp -r CosmicRays_artefacts/Release/AU/Cosmic\ Rays.component "$AU_PATH/"
    
    echo "Installing VST3 to $VST3_PATH..."
    cp -r CosmicRays_artefacts/Release/VST3/Cosmic\ Rays.vst3 "$VST3_PATH/"
    
    echo "Cosmic Rays installation complete!"
    echo "You may need to restart your DAW (e.g., GarageBand, Logic) to see the plugin."
fi
