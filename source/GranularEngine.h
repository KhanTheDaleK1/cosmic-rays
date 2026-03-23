/*
  ==============================================================================
    Cosmic Rays - Granular Engine
    Copyright (C) 2026 Very Good

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.fsf.org/licenses/>.
  ==============================================================================
*/
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <random>
#include "./helpers/Helpers.h"
#include "./dsp/BufferReaders.h"

/**
 * Grain Structure
 * Represents a single micro-loop with ballistic pitch trajectory support.
 */

struct GrainParams
{
    bool globalRev;
    bool globalQuant;
    float repeats;
    float spray;
    float spread;
    float jitter;
    float revProb;
    int globalWin;
};

class GranularEngine {
public:
    GranularEngine();
    
    /**
     * Initializes all buffers and DSP chains.
     * @param numChannels Maximum channels supported (max of input/output)
     */
    void prepare(double sampleRate, int samplesPerBlock, int numChannels, juce::AudioProcessorValueTreeState& apvts);
    
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
    float getOutputFollower() const { return outputFollower; }

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
    void scheduleGrains(float activity, float timeMs, float shape, int algo, int currentWriteIdx, const GrainParams& gp);

    // --- Core Memory Partitions ---
    juce::AudioBuffer<float> delayBuffer; // Primary real-time ring buffer
    juce::AudioBuffer<float> holdBuffer;  // Frozen sampler partition
    juce::AudioBuffer<float> loopBuffer;  // Dedicated Phrase Looper storage
    //juce::AudioBuffer<float> mdlBuffer;   // Global Pitch Modulation buffer (8192 samples)

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> mdlDelayL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> mdlDelayR;

    // --- State Indices ---
    int writeIdx = 0;
    int holdWriteIdx = 0;
    //int mdlWriteIdx = 0;
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
    float outputFollower = 0.0f;
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
    // --- Audio Thread Safe Parameter Caching ---
    std::atomic<float>* pActivity = nullptr;
    std::atomic<float>* pTime = nullptr;
    std::atomic<float>* pShape = nullptr;
    std::atomic<float>* pRepeats = nullptr;
    std::atomic<float>* pFilter = nullptr;
    std::atomic<float>* pSpace = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pGain = nullptr;
    std::atomic<float>* pMasterWet = nullptr;
    std::atomic<float>* pLoopLevel = nullptr;
    std::atomic<float>* pSpray = nullptr;
    std::atomic<float>* pSpread = nullptr;
    std::atomic<float>* pPitchJitter = nullptr;
    std::atomic<float>* pRevProb = nullptr;
    std::atomic<float>* pModRate = nullptr;
    std::atomic<float>* pModDepth = nullptr;
    std::atomic<float>* pAlgo = nullptr;
    std::atomic<float>* pFreeze = nullptr;
    std::atomic<float>* pResonance = nullptr;
    std::atomic<float>* pLooperQuant = nullptr;
    std::atomic<float>* pLooperRev = nullptr;
    std::atomic<float>* pLooperWindowType = nullptr;

    juce::AudioBuffer<float> wetBuffer;
    
    Helpers helpers;
    int recordedSamples = 0;
    BufferReaders bufferReaders;
};
