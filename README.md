# Cosmic Rays

Cosmic Rays is a cross-platform (macOS/Linux) audio plugin (AU/VST3/LV2) that provides advanced granular synthesis and glitch effects. It features a high-density granular engine with tape-style delay, rhythmic sequencing, and dynamic pattern generation.

**Version:** Beta (March 3, 2026)

## Features
- **11 Unique Algorithms**:
  - *Micro Loop*: Mosaic, Seq, Glide
  - *Granules*: Haze, Tunnel, Strum
  - *Glitch*: Blocks, Interrupt, Arp
  - *Multidelay*: Pattern, Warp
- **Advanced Granular Engine**: Dynamic envelope shaping (Sine, Tri, Saw, Square, Rand), pitch modulation, rhythmic quantizing, and reverse capabilities.
- **Hardware-Style DSP**: Integrated Reverb and analog-style Ladder Filter (logarithmic LPF/HPF).
- **Phase Looper Controls**: Reverse playback and Quantize (locks triggers to the tempo grid).
- **UI**: Modern teal/grey responsive interface with Tap Tempo LED and multi-function parameter mappings (Shift controls).
- **Cross-Platform**: Support for AU, VST3, and LV2 formats.

## Quick Installation

[![Download Cosmic Rays Beta](https://img.shields.io/badge/DOWNLOAD-Cosmic%20Rays%20Beta%20(macOS%20%26%20Linux)-009DDC?style=for-the-badge&logo=github)](https://github.com/KhanTheDaleK1/cosmic-rays/raw/main/CosmicRays_Beta_3-3-2026.zip)

*Windows installer coming soon.*

- **macOS**: Extract the zip and double-click the `Install_CosmicRays.app` for a 1-click terminal-free installation.
  - *Note for macOS users:* If you see a message saying the app "cannot be opened because it is from an unidentified developer," go to **System Settings > Privacy & Security**, scroll down to the "Security" section, and click **"Open Anyway"**.
- **Linux**: Extract and double-click the included `.desktop` or `install.sh` file.

## Build Instructions (For Source)
### Prerequisites
- CMake (3.15 or later)
- C++17 compiler (GCC/Clang)
- **Linux**: `libasound2-dev libjack-jackd2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype6-dev libcurl4-openssl-dev libwebkit2gtk-4.1-dev`

### Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## License
MIT