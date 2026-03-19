# Contributing to Cosmic Rays

Thank you for your interest in contributing to **Cosmic Rays**! This project is a professional-grade granular processor built with JUCE 8 and C++20. To maintain the highest standards of audio performance and codebase integrity, please adhere to the following guidelines.

## 1. Real-Time Safety (The Golden Rule)

Cosmic Rays is a real-time audio plugin. Performance is critical to prevent audio dropouts (glitches).
- **The Audio Thread is Sacred:** Never perform memory allocations (`new`, `malloc`, `std::make_unique`), mutex locking, file I/O, or system calls inside the `processBlock` method or any DSP code called by it.
- **Communication:** Use lock-free data structures (like `juce::AbstractFifo` or atomics) to communicate between the UI thread and the Audio thread.
- **Deterministic Math:** Ensure all DSP calculations are deterministic and optimized. Use `float` for final audio buffers but `double` for sensitive internal calculus (like phase integration or ODE solvers).

## 2. Development Workflow

### Requirements
- **CMake 3.24+**
- **JUCE 8.0.6** (managed via CPM)
- **C++20 Compatible Compiler** (MSVC 2022, Clang 15+, or GCC 12+)
- **Python 3.9+** (for versioning scripts)

### Branching Strategy
- `main` / `master`: Production-ready code. Every push here triggers an automated release and Gumroad sync.
- `feature/feature-name`: For new DSP algorithms or UI components.
- `bugfix/issue-name`: For resolving reported bugs.

## 3. Versioning & Releases

We use a strict **Timestamp-Based Versioning** system: `dd.mm.yyyy-tttt` (Central Time).
- **Automation:** Do not manually edit `Source/Version.h` or `version.txt`. 
- **Trigger:** Run `python3 Scripts/generate_version.py` before committing significant changes. This script automatically:
  1. Updates the C++ version header.
  2. Updates the project version file.
  3. Synchronizes the version badge on the documentation website (`docs/index.html`).

## 4. Coding Standards

- **Naming:** 
  - Classes/Types: `CamelCase`
  - Variables/Functions: `camelCase`
- **Formatting:** 4-space indentation. Braces on new lines (Allman style) to remain consistent with the JUCE framework.
- **Namespaces:** Always explicitly use the `juce::` namespace (e.g., `juce::AudioBuffer<float>`).
- **Math:** Use standard library math functions (`std::sin`, `std::exp`, etc.) and be explicit about floating-point precision.

## 5. Automated CI/CD Pipeline

Every push to the `main` branch triggers a GitHub Action that:
1. Compiles the plugin for **macOS, Windows, and Linux**.
2. Packages them into native installers (`.pkg`, `.exe`, `.deb`).
3. Creates a unique **Historical Release** tag (e.g., `v19.03.2026-0905`).
4. Updates the **Latest Release** tag with clean, generic filenames.
5. Syncs the new installers directly to **Gumroad**.

## 6. How to Submit a Contribution

1. **Fork the Repository** and create your branch from `main`.
2. **Implement Tests:** If adding a new DSP feature, ensure you provide a way to verify its output (e.g., a simple test signal or internal assertions).
3. **Run Linting:** Ensure your code matches the existing style.
4. **Submit a Pull Request:** Provide a clear description of the changes and any mathematical rationale for DSP modifications.

---
*“In the granular void, precision is our only anchor.”*
