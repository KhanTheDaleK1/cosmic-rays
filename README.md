# Cosmic Rays (2026 Edition)

**Official Website:** [beechem.site/cosmic-rays](https://beechem.site/cosmic-rays)

Cosmic Rays is a professional-grade granular processor for macOS, Linux, and Windows, inspired by boutique hardware granular synthesis. It utilizes advanced kinematic motion models and a dedicated global modulation engine to create lush, nostalgic, and evolving sonic textures.

## 2026 Project Standards
This project follows modern C++20 and JUCE 8.0.6 best practices:
- **Build System:** CMake 3.24+ with [CPM.cmake](https://github.com/cpm-cmake/CPM) for seamless dependency management.
- **Hardware Acceleration:** Native Direct2D (Windows) and Metal (macOS) rendering for ultra-low latency GUIs.
- **State Management:** Type-safe `juce::AudioProcessorValueTreeState` integration.
- **CI/CD:** Automated multi-platform builds via GitHub Actions with OpenSSF security auditing.
- **AI-Ready:** Includes `.github/copilot-instructions.md` for enhanced development with AI agents.

## DSP Architecture

### 1. High-Performance Granular Engine
- **256-Voice Grain Pool:** Expanded synthesis engine supporting up to 256 simultaneous grains.
- **Unity Gain Standard:** Hybrid Saturated Normalization (Linear normalization + Tanh safety stage).
- **Exponential Pitch Shifting:** All pitch modulations follow musical semitone scaling ($2^{n/12}$).
- **Kinematic Grain Motion:** Grains follow ballistic trajectories for liquid-smooth glissandos.

### 2. Global Modulation Engine (MDL)
- **JUCE-Optimized Delay Lines:** High-fidelity tape emulation via `juce::dsp::DelayLine`.
- **Prime-Ratio LFOs:** Three oscillators at prime-number ratios for organic "tape drift."
- **Broken Capstan Algorithm:** Asymmetrical LFO curves for unstable motor simulation.

## Build & Installation

### Build from Source
Ensure you have **CMake 3.24+** and a C++20 compatible compiler (Clang 15+, GCC 12+, or MSVC 2022+).

```bash
# Clone and build
git clone https://github.com/evanbeechem/cosmic-rays.git
cd cosmic-rays
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

- **macOS/Linux:** Use `./install.sh` for quick setup.
- **Windows:** Use `install.bat` for quick setup.

## Development
- **Issue Templates:** Use the YAML-based issue forms for bug reports and feature requests.
- **Dependency Updates:** Managed automatically via Renovate.
- **Legacy Support:** The `.jucer` file is provided for reference but CMake is the mandatory build path.

## License
MIT
