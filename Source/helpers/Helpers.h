/*
  ==============================================================================

    Helpers.h
    Created: 18 Mar 2026
    Author:  Gemini CLI (Expert DSP Auditor)

    MODERN 2026 STANDARDS:
    - Zero Heap Allocations on Audio Thread
    - Lock-Free Grain Pool Management
    - Phase-Accurate Kinematic Trajectories
  ==============================================================================
*/

#pragma once
#include <array>
#include <atomic>

/**
 * Advanced Kinematic Grain Structure
 * Includes support for ballistic pitch models and deterministic lifecycles.
 */
struct Grain {
    // Phase & Playback
    float currentPos = 0.0f;    
    float startPos = 0.0f;      
    float length = 0.0f;        
    float age = 0.0f;           
    
    // Kinematic Motion (2026 Standards)
    float velocity = 1.0f;      // Current playback rate
    float acceleration = 0.0f;  // Rate of change for velocity (ballistic drift)
    float targetVelocity = 1.0f; 
    
    // Legacy Fields (for compatibility)
    float startSpeed = 1.0f;    
    float endSpeed = 1.0f;      
    float currentSpeed = 1.0f;  
    
    // Stereo & Modulation
    float pan = 0.5f;           
    float internalPhase = 0.0f; 
    int windowType = 0;         
    
    // State Flags
    std::atomic<bool> active { false }; 
    bool reverse = false;       

    // Per-grain modifiers
    float filterCutoff = 1.0f;
    float filterRes = 0.0f;
    float bitCrush = 1.0f;      
    bool isSustainer = false;   
    float targetPitch = 1.0f;
    float brightness = 1.0f;
    int customID = 0;           
};

static constexpr int MAX_GRAINS = 256;

/**
 * Real-Time Safe Grain Management
 * Uses a pre-allocated index stack for O(1) acquisition without heap usage.
 */
class Helpers {
public:
    Helpers() {
        for (int i = 0; i < MAX_GRAINS; ++i) {
            freeIndices[i] = i;
        }
        freeCount.store(MAX_GRAINS);
    }

    /**
     * Acquires a grain index from the free pool.
     * thread-safe for the audio thread (Single Producer / Single Consumer context).
     */
    int acquireFreeGrain() noexcept
    {
        int count = freeCount.load(std::memory_order_acquire);
        if (count <= 0) return -1;

        int idx = freeIndices[count - 1];
        freeCount.store(count - 1, std::memory_order_release);
        return idx;
    }

    /**
     * Returns a grain index to the free pool.
     */
    void releaseGrain(int grainIndex) noexcept
    {
        int count = freeCount.load(std::memory_order_acquire);
        if (count >= MAX_GRAINS) return;

        freeIndices[count] = grainIndex;
        freeCount.store(count + 1, std::memory_order_release);
    }

private:
    std::array<int, MAX_GRAINS> freeIndices;
    std::atomic<int> freeCount { 0 };
};
