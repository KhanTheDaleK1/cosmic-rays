# Cosmic Rays

**Official Website:** [beechem.site/cosmic-rays](https://beechem.site/cosmic-rays) | **Gumroad:** [Cosmic-Rays Beta](https://verygoodideas.gumroad.com/l/Cosmic-Rays)

Cosmic Rays is a professional-grade granular processor for macOS, Linux, and Windows, inspired by boutique hardware granular synthesis. It utilizes advanced kinematic motion models and a dedicated global modulation engine to create lush, nostalgic, and evolving sonic textures.

<img width="659" height="498" alt="Screenshot 2026-03-16 141452" src="https://github.com/user-attachments/assets/e0a3e3ea-77ff-4fbf-99bf-ddbc8a8994b7" />

## 2026 Project Standards
This project follows modern C++20 and JUCE 8.0.6 best practices:
- **Build System:** CMake 3.27+ with [CPM.cmake](https://github.com/cpm-cmake/CPM) for seamless dependency management.
- **Real-Time Safety:** All DSP code adheres to strict lock-free and allocation-free mandates on the audio thread.
- **Hardware Acceleration:** Native Direct2D (Windows) and Metal (macOS) rendering for ultra-low latency GUIs.
- **Automated CI/CD:** Integrated multi-platform builds via GitHub Actions with automated **Gumroad** synchronization.
- **Versioning:** Strict timestamp-based versioning (`dd.mm.yyyy-tttt`) synced across code, website, and releases.

## DSP Architecture

### 1. High-Performance Granular Engine
- **256-Voice Grain Pool:** Expanded synthesis engine supporting up to 256 simultaneous grains.
- **Unity Gain Standard:** Hybrid Saturated Normalization (Linear normalization + Tanh safety stage).
- **Exponential Pitch Shifting:** All pitch modulations follow musical semitone scaling ($2^{n/12}$).
- **Kinematic Grain Motion:** Grains follow physics-modeled ballistic trajectories for liquid-smooth glissandos.

### 2. Global Modulation Engine (MDL)
- **JUCE-Optimized Delay Lines:** High-fidelity tape emulation via `juce::dsp::DelayLine`.
- **Prime-Ratio LFOs:** Three oscillators at prime-number ratios for organic "tape drift."
- **Broken Capstan Algorithm:** Asymmetrical LFO curves for unstable motor simulation.

## Automated Release Ecosystem

Cosmic Rays features a fully automated deployment pipeline. Every push to `main` triggers:
1.  **Multi-OS Compilation:** Native installers (`.pkg`, `.exe`, `.deb`) are generated for all platforms.
2.  **Latest Release Sync:** The `latest` GitHub release tag is updated with clean, generic installer filenames.
3.  **Historical Archive:** A unique, timestamped version tag is created for every build.
4.  **Gumroad Automation:** The latest installers are automatically synchronized to the Gumroad product page.

## Build & Installation

### Build from Source
Ensure you have **CMake 3.27+** and a C++20 compatible compiler (Clang 15+, GCC 12+, or MSVC 2022+).

```bash
# Clone and build
git clone https://github.com/KhanTheDaleK1/cosmic-rays.git
cd cosmic-rays
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

- **macOS/Linux:** Use `./install.sh` for quick setup.
- **Windows:** Use `install.bat` for quick setup.

## Development & Contribution

We welcome community contributions! Please review our **[CONTRIBUTING.md](CONTRIBUTING.md)** and **[SECURITY.md](SECURITY.md)** before submitting pull requests.

- **Versioning:** Run `python3 Scripts/generate_version.py` before significant commits to sync the project versioning.
- **Issue Templates:** Use specialized YAML forms for **[Bug Reports](https://github.com/KhanTheDaleK1/cosmic-rays/issues/new?template=bug_report.yml)**, **[Feature Requests](https://github.com/KhanTheDaleK1/cosmic-rays/issues/new?template=feature_request.yml)**, and **[DSP Artifacts](https://github.com/KhanTheDaleK1/cosmic-rays/issues/new?template=dsp_artifact.yml)**.
- **Dependencies:** Managed automatically via **Dependabot**.

## License
MIT
