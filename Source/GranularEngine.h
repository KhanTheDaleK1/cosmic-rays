/**
 * COSMIC RAYS - Granular Engine Header
 * -----------------------------------
 * Advanced kinematic granular synthesis engine with ballistic motion models
 * and a global Modulated Delay Line (MDL) for vintage tape emulation.
 * 
 * Standards: C++17, JUCE 8.0+
 */

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <random>

/**
 * Grain Structure
 * Represents a single micro-loop with ballistic pitch trajectory support.
 */
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
};

class GranularEngine {
public:
    GranularEngine();
    
    /**
     * Initializes all buffers and DSP chains.
     * @param numChannels Maximum channels supported (max of input/output)
     */
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    
    /**
     * Main processing loop called by the AudioProcessor.
     */
    void processBlock(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts);

    // Getters for Visualizers
    const std::vector<Grain>& getGrains() const { return grains; }
    float getActiveGrainCount() const { return currentActiveGrainCount; }
    const juce::AudioBuffer<float>& getDelayBuffer() const { return delayBuffer; }
    int getWriteIdx() const { return writeIdx; }
    float getEnvFollower() const { return envFollower; }

private:
    /**
     * Cubic interpolation for high-quality pitch shifting without aliasing.
     */
    float getNextSampleCubic(int channel, float readPos);
    
    /**
     * Analyzes input signal for transient-driven grain triggering.
     */
    void updateEnvelopeFollower(const juce::AudioBuffer<float>& buffer);
    
    /**
     * Handles the logic for spawning new grains based on selected algorithm.
     */
    void scheduleGrains(float activity, float timeMs, float shape, int algo, int currentWriteIdx, juce::AudioProcessorValueTreeState& apvts);

    // --- Core Memory Partitions ---
    juce::AudioBuffer<float> delayBuffer; // Primary real-time ring buffer
    juce::AudioBuffer<float> holdBuffer;  // Frozen sampler partition
    juce::AudioBuffer<float> loopBuffer;  // Dedicated Phrase Looper storage
    juce::AudioBuffer<float> mdlBuffer;   // Global Pitch Modulation buffer (8192 samples)
    
    // --- State Indices ---
    int writeIdx = 0;
    int holdWriteIdx = 0;
    int mdlWriteIdx = 0;
    int preparedChannels = 2;
    int loopWritePos = 0, loopReadPos = 0, loopLength = 0;
    
    // --- Physics & LFO State ---
    float mdlPhase = 0.0f, mdlPhase2 = 0.0f, mdlPhase3 = 0.0f;
    float grainClock = 0.0f;
    int stepCount = 0;
    bool glideFlip = false;
    bool holdLocked = false;
    bool isOverdubbing = false;
    
    // --- DSP Components ---
    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<float>, juce::dsp::Reverb, juce::dsp::Chorus<float>> fxChain;
    juce::dsp::LadderFilterMode currentFilterMode;
    std::vector<std::unique_ptr<juce::dsp::DelayLine<float>>> diffusers;
    std::vector<float> diffuserPhases;
    const int numDiffusers = 4;
    
    juce::dsp::IIR::Filter<float> tiltFilterL, tiltFilterR;
    juce::dsp::Limiter<float> limiter;
    
    // --- Internal Modulation ---
    float envFollower = 0.0f;
    bool transientDetected = false;
    float noiseMod = 0.0f, noiseTarget = 0.0f;
    float currentActiveGrainCount = 0.0f;
    float interruptGate = 1.0f;
    int interruptTimer = 0;
    float sampleAccumulator = 0.0f;
    float sampleRateReducer = 0.0f;

    // --- Parameter Smoothing ---
    juce::LinearSmoothedValue<float> smoothActivity, smoothTime, smoothShape, smoothRepeats, 
                                     smoothFilter, smoothSpace, smoothMix, smoothGain, 
                                     smoothLoopLevel, smoothDrift, smoothSnap, 
                                     smoothMasterWetVol, smoothLoopFade, smoothSpray, 
                                     smoothSpread, smoothEnvFollower, smoothPitchJitter, 
                                     smoothRevProb, smoothModRate, smoothModDepth;

    // --- DC Offset Prevention ---
    float dcBlockerX_L = 0.0f, dcBlockerY_L = 0.0f;
    float dcBlockerX_R = 0.0f, dcBlockerY_R = 0.0f;
    
    struct FilterState { float x1 = 0, y1 = 0; };
    std::vector<std::vector<FilterState>> grainFilters;

    std::vector<Grain> grains;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    double fs = 44100.0;
    int maxDelaySamples = 0, maxHoldSamples = 0;
};
