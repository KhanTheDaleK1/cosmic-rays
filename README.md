# Cosmic Rays

Cosmic Rays is a cross-platform (macOS/Linux) audio plugin (AU/VST3/LV2) that emulates the granular synthesis and glitch effects of the Hologram Microcosm pedal.

## Features (Planned)
- Granular delay and loop slicing.
- Rhythmic pattern generation.
- Pitch shifting and time-stretching.
- Reverb and Modulation.
- DAW integration for macOS (GarageBand) and Linux (Zrythm).

## Build Instructions

### Prerequisites
- CMake (3.15 or later)
- C++17 compiler (GCC/Clang)
- For Linux: `libasound2-dev libjack-jackd2-dev libx11-dev libxext-dev libxinerama-dev libxrandr-dev libxcursor-dev libfreetype6-dev libcurl4-openssl-dev`

### Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## License
MIT
