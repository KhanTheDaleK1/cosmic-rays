#include "GranularEngine.h"
#include <cmath>

GranularEngine::GranularEngine() : rng(std::random_device{}()), dist(0.0f, 1.0f) {
    grains.resize(64); // Support high grain density for Portal/Fragments-like sounds
}

void GranularEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
    juce::ignoreUnused(samplesPerBlock);
    fs = sampleRate;
    maxDelaySamples = static_cast<int>(sampleRate * 5.0); // 5 seconds of maximum delay buffer
    if (numChannels == 0) numChannels = 2; // Default to stereo fallback
    delayBuffer.setSize(numChannels, maxDelaySamples);
    delayBuffer.clear();
    writeIdx = 0;
}

float GranularEngine::getNextSample(int channel, float readPos) {
    if (channel >= delayBuffer.getNumChannels()) channel = 0;
    int idx1 = static_cast<int>(readPos);
    int idx2 = (idx1 + 1) % maxDelaySamples;
    float frac = readPos - idx1;
    
    float val1 = delayBuffer.getSample(channel, idx1);
    float val2 = delayBuffer.getSample(channel, idx2);
    
    // Linear interpolation
    return val1 + frac * (val2 - val1);
}

void GranularEngine::scheduleGrains(float density, float sizeMs, float pitchSemitones, float reverseProb, float spray) {
    // Basic stochastic scheduler: the higher the density, the higher the chance per sample frame
    float probTrigger = (density / 100.0f) * 0.1f; 
    
    if (dist(rng) < probTrigger) {
        for (auto& g : grains) {
            if (!g.active) {
                g.active = true;
                g.age = 0.0f;
                g.length = (sizeMs / 1000.0f) * fs;
                g.speed = std::pow(2.0f, pitchSemitones / 12.0f);
                g.reverse = (dist(rng) < reverseProb);
                
                // Random offset into the delay buffer based on "spray"
                float offset = dist(rng) * fs * 1.0f; // Up to 1s into the past
                g.currentPos = writeIdx - offset;
                if (g.currentPos < 0) g.currentPos += maxDelaySamples;
                
                g.pan = 0.5f + (dist(rng) * 2.0f - 1.0f) * spray * 0.5f;
                break;
            }
        }
    }
}

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts) {
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    if (numChannels == 0) return;
    
    // Read new macro parameters
    float density = apvts.getRawParameterValue("DENSITY")->load();
    float sizeMs = apvts.getRawParameterValue("GRAIN_SIZE")->load();
    float pitch = apvts.getRawParameterValue("PITCH")->load();
    float feedback = apvts.getRawParameterValue("FEEDBACK")->load();
    float mix = apvts.getRawParameterValue("MIX")->load();
    float reverseProb = apvts.getRawParameterValue("REVERSE")->load();
    float spray = apvts.getRawParameterValue("SPRAY")->load();

    juce::AudioBuffer<float> wetBuffer;
    wetBuffer.setSize(numChannels, numSamples);
    wetBuffer.clear();

    for (int i = 0; i < numSamples; ++i) {
        scheduleGrains(density, sizeMs, pitch, reverseProb, spray);
        
        float currentWetL = 0.0f;
        float currentWetR = 0.0f;
        
        // Process Active Grains
        for (auto& g : grains) {
            if (g.active) {
                // Apply Hann Window for smooth grain amplitude
                float windowPhase = g.age / g.length;
                float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * windowPhase));
                
                float posIncr = g.reverse ? -g.speed : g.speed;
                g.currentPos += posIncr;
                
                // Wrap positions
                if (g.currentPos >= maxDelaySamples) g.currentPos -= maxDelaySamples;
                if (g.currentPos < 0) g.currentPos += maxDelaySamples;
                
                float sL = getNextSample(0, g.currentPos) * window;
                float sR = getNextSample(numChannels > 1 ? 1 : 0, g.currentPos) * window;
                
                // Pan and Accumulate
                currentWetL += sL * (1.0f - g.pan);
                currentWetR += sR * g.pan;
                
                g.age += 1.0f;
                if (g.age >= g.length) {
                    g.active = false; // Retire grain
                }
            }
        }
        
        // Write to buffer and handle Feedback
        for (int ch = 0; ch < numChannels; ++ch) {
            float inSample = buffer.getSample(ch, i);
            float fbSample = (ch == 0) ? currentWetL : currentWetR;
            // Soft clipping on the feedback loop to prevent massive signal explosions
            float writeVal = inSample + (fbSample * feedback);
            writeVal = std::tanh(writeVal);
            delayBuffer.setSample(ch, writeIdx, writeVal);
        }
        
        writeIdx = (writeIdx + 1) % maxDelaySamples;
        
        wetBuffer.setSample(0, i, currentWetL);
        if (numChannels > 1) wetBuffer.setSample(1, i, currentWetR);
    }
    
    // Mix Dry/Wet
    for (int ch = 0; ch < numChannels; ++ch) {
        auto* dryData = buffer.getWritePointer(ch);
        auto* wetData = wetBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i) {
            dryData[i] = (dryData[i] * (1.0f - mix)) + (wetData[i] * mix);
        }
    }
}