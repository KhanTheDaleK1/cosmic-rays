# Cosmic Rays (Beta 3-12-2026)

**Official Website:** [beechem.site/cosmic-rays](https://beechem.site/cosmic-rays)

Cosmic Rays is a professional-grade granular processor for macOS, Linux, and Windows, inspired by boutique hardware granular synthesis. It utilizes advanced kinematic motion models and a dedicated global modulation engine to create lush, nostalgic, and evolving sonic textures.

## New Aesthetic: "The Time Machine" (Beta 3-12-2026)
- **80s Hardware Housing:** A meticulously textured "aged plastic" enclosure with authentic orange-peel bumps, scratches, and tasteful cracking.
- **Chrome-Accented Scopes:** Three round, phosphor-green displays for Waveform, Pitch Cloud, and Filter response, now featuring machined chrome bezels.
- **Protruding Analog Meter:** A redesigned physical needle-based meter for monitoring grain density with realistic depth and glass lens reflections.
- **Vertical Channel Strip Layout:** Controls are now logically organized into vertical functional columns (Source, Algo, Tone, Master) for intuitive "signal flow" operation.
- **Global Chrome Border:** A 4px wide metallic chrome frame around the entire plugin window.

<img width="724" height="542" alt="image" src="https://github.com/user-attachments/assets/3193cff0-00e3-463f-ac2f-b1994c40d54a" />

## Key Features

### 1. High-Performance Granular DSP
- **256-Voice Grain Pool:** Expanded synthesis engine supporting up to 256 simultaneous grains for ultra-dense textures without voice stealing.
- **Unity Gain Standard:** Hybrid Saturated Normalization (Linear normalization + Tanh safety stage) ensures that output volume always matches input volume regardless of density.
- **Exponential Pitch Shifting:** All pitch modulations (Glide, Jitter, Drift) follow musical semitone scaling ($2^{n/12}$) for perfect harmonic consistency.
- **O(1) Pool-Based Grain Management:** Optimized acquisition and release using an index-swapping pool system for ultra-low CPU overhead.
- **Kinematic Grain Motion:** Grains follow ballistic trajectories, delivering liquid-smooth pitch glissandos without digital artifacts.

### 2. Global Modulation Engine (MDL)
- **JUCE-Optimized Delay Lines:** Integration with `juce::dsp::DelayLine` for stable, high-fidelity pitch modulation and vintage tape emulation.
- **Prime-Ratio LFOs:** Summation of three oscillators at prime-number ratios for non-repeating, organic "tape drift."
- **Broken Capstan Algorithm:** Asymmetrical LFO curves combined with deep delay lines (up to 50ms) simulate the lurching instability of failing tape motors.
- **Musical Warble:** A high-range (up to 8Hz) vibrato stage with amplitude surging for nostalgic shimmer.

### 3. Professional UI/UX
- **Retro-Modern Visualizers:** 1950s oscilloscope-style scopes and an analog VU meter.
- **Power-User Shortcuts:** Full Undo/Redo (Cmd/Ctrl+Z), instant algorithm switching (1-0), and high-precision Fine-Tune mode (Shift).
- **Performance HUD:** Real-time CPU thread load and RAM footprint monitoring.

## Installation

### 1-Click Install (From Source)
Ensure you have **CMake** installed.

- **macOS/Linux:** Run `./install.sh` in your terminal.
- **Windows:** Run `install.bat` in your terminal.

## Build Requirements
- **JUCE 8.0+**
- **CMake 3.15+** (or Projucer)
- **Compiler:** Clang (macOS), GCC (Linux), or MSVC (Windows - VS2022) with C++17 support.

## License
MIT
