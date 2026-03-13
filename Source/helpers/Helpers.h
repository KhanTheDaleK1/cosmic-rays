/*
  ==============================================================================

    helpers.h
    Created: 10 Mar 2026 10:11:06am
    Author:  diego

  ==============================================================================
*/

#pragma once
#include <vector>

struct Grain {
    float currentPos = 0.0f;    // Current read position in the delay buffer
    float startPos = 0.0f;      // Original trigger position (for kinematic calcs)
    float length = 0.0f;        // Total duration in samples
    float startSpeed = 1.0f;    // Base playback rate (unity = 1.0)
    float endSpeed = 1.0f;      // Target playback rate (or depth overloading)
    float internalPhase = 0.0f; // Phase offset for internal 'wah' oscillations
    float pan = 0.5f;           // Stereo position (0.0 = Left, 1.0 = Right)
    float age = 0.0f;           // Current age in samples
    int windowType = 0;         // Envelope shape index
    bool active = false;        // Trigger status
    bool reverse = false;       // Playback direction

    // Per-grain modifiers
    float filterCutoff = 1.0f;
    float filterRes = 0.0f;
    float bitCrush = 1.0f;      // 1.0 = no crush, < 1.0 = reduction
    bool isSustainer = false;   // Special flag for SEQ-mode sustain grains
    
    // 2026 Enhanced Fields
    float targetPitch = 1.0f;
    float brightness = 1.0f;
    int customID = 0;           // For sequencing/arp steps
};

class Helpers {
public:
    int acquireFreeGrain(std::vector<int>& activeGrainIndices, std::vector<int>& freeGrainIndices, std::vector<Grain>& grains) noexcept
    {
        if (freeGrainIndices.empty())
            return -1;

        const int idx = freeGrainIndices.back();
        freeGrainIndices.pop_back();

        activeGrainIndices.push_back(idx);
        grains[(size_t)idx].active = true;
        return idx;
    }

    void releaseActiveGrainByActiveListPosition(
    std::vector<int>& activeGrainIndices, 
    std::vector<int>& freeGrainIndices, 
    std::vector<Grain>& grains, 
    int activeListPos) noexcept
    {
        auto& active = activeGrainIndices;
        const int grainIdx = active[(size_t)activeListPos];

        grains[(size_t)grainIdx].active = false;
        freeGrainIndices.push_back(grainIdx);

        const int last = (int)active.size() - 1;
        if (activeListPos != last)
            active[(size_t)activeListPos] = active[(size_t)last];

        active.pop_back();
    }
};
