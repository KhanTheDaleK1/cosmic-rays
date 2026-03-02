#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <random>

struct Grain {
    float currentPos = 0.0f;
    float length = 0.0f;
    float speed = 1.0f;
    float pan = 0.5f;
    float age = 0.0f;
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
    void scheduleGrains(float density, float sizeMs, float pitch, float reverseProb, float spray);

    double fs = 44100.0;
    juce::AudioBuffer<float> delayBuffer;
    int writeIdx = 0;
    int maxDelaySamples = 0;
    
    std::vector<Grain> grains;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
};