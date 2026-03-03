#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <random>

struct Grain {
    float currentPos = 0.0f;
    float length = 0.0f;
    float startSpeed = 1.0f;
    float endSpeed = 1.0f;
    float pan = 0.5f;
    float age = 0.0f;
    int windowType = 0; // 0: Sine, 1: Tri, 2: Saw, 3: Square, 4: Rand
    bool active = false;
    bool reverse = false;
};

class GranularEngine {
public:
    GranularEngine();
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts);

private:
    float getNextSample(int channel, float readPos);
    void scheduleGrains(float activity, float timeMs, float shape, int algo, int currentWriteIdx, juce::AudioProcessorValueTreeState& apvts);

    double fs = 44100.0;
    juce::AudioBuffer<float> delayBuffer;
    int writeIdx = 0;
    int maxDelaySamples = 0;
    
    std::vector<Grain> grains;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    
    // Algorithm & Rhythm State
    float grainClock = 0.0f;
    int stepCount = 0;

    // Phase Looper State
    juce::AudioBuffer<float> loopBuffer;
    int loopWritePos = 0;
    int loopReadPos = 0;
    int loopLength = 0;
    bool isOverdubbing = false;

    // Wow & Flutter / Modulation
    float modPhase = 0.0f;
    float modRate = 0.5f;
    float modDepth = 0.1f;

    // Global Modes
    bool killDry = false;
    bool trailsEnabled = true;
    float bypassVolume = 1.0f;

    // Hardware-style DSP
    juce::dsp::ProcessorChain<juce::dsp::LadderFilter<float>, juce::dsp::Reverb, juce::dsp::Chorus<float>> fxChain;
    juce::dsp::LadderFilterMode currentFilterMode = juce::dsp::LadderFilterMode::LPF24;

    // Smoothed Parameters
    juce::LinearSmoothedValue<float> smoothActivity, smoothTime, smoothShape, smoothRepeats, smoothFilter, smoothSpace, smoothMix, smoothGain, smoothLoopLevel;
    juce::LinearSmoothedValue<float> smoothDrift, smoothSnap;

    // DC Blocker State
    float dcBlockerX_L = 0.0f, dcBlockerY_L = 0.0f;
    float dcBlockerX_R = 0.0f, dcBlockerY_R = 0.0f;
};