# Copilot Instructions: Cosmic Rays (DSP & JUCE 8)

## Architecture & Style
- **DSP Thread Safety:** Never allocate memory, lock mutexes, or perform I/O in `processBlock`. Use lock-free buffers or atomic primitives for UI/Audio thread communication.
- **JUCE 8 Standards:** Use `juce::AudioProcessorValueTreeState` for parameter management. Prefer modern C++20/23 features when possible (ranges, concepts).
- **Type-Safe ValueTrees:** Favor type-safe wrappers for `ValueTree` property access to avoid string-key errors.
- **Granular Synthesis:** Focus on mathematical precision (using `double` for phase/integration) vs. performance (using `float` for final audio buffers).

## Coding Conventions
- **Naming:** CamelCase for classes/types, camelCase for variables/functions.
- **Namespaces:** Explicitly use `juce::` namespace for all JUCE classes.
- **RAII:** Leverage JUCE's `ScopedPointer` equivalents or `std::unique_ptr`/`std::shared_ptr`.
- **Formatting:** 4-space indentation, braces on new lines (Allman style) to match JUCE's codebase.

## Math & Calculus
- Use `std::` math functions (e.g., `std::sin`, `std::exp`).
- Be explicit about floating-point precision.
- Optimize non-linear functions with lookup tables or SIMD when necessary.
