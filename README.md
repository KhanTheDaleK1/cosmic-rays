# Cosmic Rays (Beta 3-6-2026)

Cosmic Rays is a professional-grade granular processor for macOS and Linux, inspired by boutique hardware granular synthesis. It utilizes advanced kinematic motion models and a dedicated global modulation engine to create lush, nostalgic, and evolving sonic textures.

<img width="660" height="494" alt="image" src="https://github.com/user-attachments/assets/bef1af24-72f3-439b-aff2-89b7f899e44a" />


## Key Features

### 1. Advanced Granular DSP
- **Kinematic Grain Motion:** Grains follow ballistic trajectories ($x = x_0 + v_0t + 0.5at^2$), delivering liquid-smooth pitch glissandos without digital artifacts.
- **Wah-Glide Model:** Sinusoidal internal pitch oscillation within each grain for organic, vocal-like movement.
- **Microcosm-Style Algorithms:** 11 unique modes including Mosaic, Seq, Glide, and Strum, precisely tuned to hardware specifications.

### 2. Global Modulation Engine (MDL)
- **Prime-Ratio LFOs:** Summation of three oscillators at prime-number ratios for non-repeating, organic "tape drift."
- **Broken Capstan Algorithm:** Asymmetrical LFO curves combined with deep delay lines (up to 50ms) simulate the lurching instability of failing tape motors.
- **Musical Warble:** A high-range (up to 8Hz) vibrato stage with amplitude surging for nostalgic shimmer.

### 3. Professional UI/UX
- **Real-time Waveform Visualizer:** See exactly where grains are being sampled in the live buffer.
- **Dynamic Density Meter:** Monitor engine load and active grain count.
- **Power-User Shortcuts:** Full Undo/Redo (Cmd+Z), instant algorithm switching (1-0), and high-precision Fine-Tune mode (Shift).
- **Performance HUD:** Real-time CPU thread load and RAM footprint monitoring.

## Controls

- **Activity:** Governs trigger density and internal oscillation rate.
- **Repeats:** Scales grain lifecycle and stacking duration.
- **Shape (Variation):** Selects from 4 sub-modes (A, B, C, D) per algorithm.
- **Shift Functions:**
  - **Shift + Repeats:** Global Modulation Depth.
  - **Shift + Shape:** Global Modulation Rate.
  - **Shift + Filter:** Filter Resonance.

## Build Requirements
- **JUCE 8.0+**
- **CMake 3.15+**
- **Compiler:** Clang (macOS) or GCC (Linux) with C++17 support.

## License
MIT
