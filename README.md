# Cosmic Rays

Cosmic Rays is a cross-platform (macOS/Linux) audio plugin (AU/VST3/LV2) that emulates the granular synthesis and glitch effects of the Hologram Microcosm pedal. It features a high-density granular engine with tape-style delay and stochastic pattern generation.

## Current Features
- **Granular Synthesis Engine**: Stochastic grain scheduler with density, size, pitch, and spray controls.
- **Tape Delay Simulation**: Integrated feedback loop with tape flutter and reverse probability.
- **Multiple Algorithms**: Mosaic, Glitch, Warp, and Ghost modes for diverse sonic textures.
- **Cross-Platform**: Support for AU, VST3, and LV2 formats on macOS and Linux.
- **UI**: Modern teal/grey interface with responsive controls.

## Build Instructions

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

## Installation
Use the provided `install.sh` script for a 1-click installation on macOS or Linux.

## License
MIT
