#include "GranularEngine.h"
#include <cmath>

GranularEngine::GranularEngine() : rng(std::random_device{}()), dist(0.0f, 1.0f) {
    grains.resize(64); 
}

void GranularEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
    fs = sampleRate;
    maxDelaySamples = static_cast<int>(sampleRate * 5.0); 
    delayBuffer.setSize(numChannels, maxDelaySamples);
    delayBuffer.clear();
    writeIdx = 0;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };
    fxChain.prepare(spec);
    
    currentFilterMode = juce::dsp::LadderFilterMode::LPF24;
    fxChain.get<0>().setMode(currentFilterMode);

    smoothActivity.reset(sampleRate, 0.05);
    smoothTime.reset(sampleRate, 0.05);
    smoothShape.reset(sampleRate, 0.05);
    smoothRepeats.reset(sampleRate, 0.05);
    smoothFilter.reset(sampleRate, 0.05);
    smoothSpace.reset(sampleRate, 0.05);
    smoothMix.reset(sampleRate, 0.05);
    smoothGain.reset(sampleRate, 0.05);
    smoothLoopLevel.reset(sampleRate, 0.05);

    loopBuffer.setSize(numChannels, (int)(sampleRate * 30.0)); // 30 second loop max
    loopBuffer.clear();
    loopWritePos = 0; loopReadPos = 0; loopLength = 0;
    isRecording = false; isPlaying = false;

    dcBlockerX_L = 0.0f; dcBlockerY_L = 0.0f;
    dcBlockerX_R = 0.0f; dcBlockerY_R = 0.0f;
    for (auto& g : grains) g.active = false;
}

float GranularEngine::getNextSample(int channel, float readPos) {
    if (channel >= delayBuffer.getNumChannels()) channel = 0;
    int idx1 = static_cast<int>(readPos);
    int idx2 = (idx1 + 1) % maxDelaySamples;
    float frac = readPos - idx1;
    float v1 = delayBuffer.getSample(channel, idx1);
    float v2 = delayBuffer.getSample(channel, idx2);
    return v1 + frac * (v2 - v1);
}

void GranularEngine::scheduleGrains(float activity, float timeMs, float shape, int algo, int currentWriteIdx, juce::AudioProcessorValueTreeState& apvts) {
    bool globalRev = apvts.getRawParameterValue("LOOPER_REV")->load() > 0.5f;
    bool globalQuant = apvts.getRawParameterValue("LOOPER_QUANT")->load() > 0.5f;

    // Clock-based scheduling for rhythmic algos
    float samplesPerStep = (timeMs * (float)fs / 1000.0f) * 0.25f; // 16th note default
    if (samplesPerStep < 100.0f) samplesPerStep = 100.0f;
    
    grainClock += 1.0f;
    bool isStep = false;
    if (grainClock >= samplesPerStep) {
        grainClock -= samplesPerStep;
        stepCount = (stepCount + 1) % 16;
        isStep = true;
    }

    // Trigger Logic
    bool trigger = false;
    float prob = 0.001f + (activity * 0.05f);

    if (globalQuant) {
        trigger = isStep && (dist(rng) < activity);
    } else {
        switch (algo) {
            case 0: // Mosaic: Random micro-loops
                trigger = (dist(rng) < prob); break;
            case 1: // Seq: Rhythmic stepping
            case 2: // Glide: Smooth pitch shifting
                trigger = isStep && (dist(rng) < activity); break;
            case 3: // Haze: Random micro-textures
            case 4: // Tunnel: Reverb-like grains
                trigger = (dist(rng) < prob * 0.5f); break;
            case 5: // Strum: Burst arpeggiation
                trigger = isStep && (stepCount % 4 == 0) && (dist(rng) < activity); break;
            case 6: // Blocks: Gated rhythm
            case 7: // Interrupt: Glitchy bursts
                trigger = isStep && (dist(rng) < activity); break;
            case 8: // Arp: Pitch arpeggiation
                trigger = isStep && (dist(rng) < activity); break;
            case 9: // Pattern: Multi-tap
            case 10: // Warp: Speed warping
                trigger = isStep && (stepCount % 2 == 0); break;
        }
    }

    if (!trigger) return;

    for (auto& g : grains) {
        if (!g.active) {
            g.active = true; g.age = 0.0f; g.speed = 1.0f; 
            g.reverse = globalRev; // Respect global reverse

            // Algorithm Behaviors
            switch (algo) {
                case 1: // Seq
                    g.speed = (stepCount % 4 == 0) ? 2.0f : 1.0f; break;
                case 2: // Glide
                    g.speed = 0.5f + (shape * 1.5f); break;
                case 3: // Haze
                    g.speed = (dist(rng) < 0.2f) ? 0.5f : (dist(rng) > 0.8f ? 2.0f : 1.0f);
                    g.speed += (dist(rng) * 2.0f - 1.0f) * shape * 0.2f; break;
                case 4: // Tunnel
                    g.speed = 0.5f; break;
                case 5: // Strum
                    g.speed = 1.0f + (float)(stepCount % 8) * 0.125f; break;
                case 6: // Blocks
                    g.reverse = globalRev ? (stepCount % 4 != 0) : (stepCount % 4 == 0); break;
                case 7: // Interrupt
                    if (!globalRev) g.reverse = (dist(rng) < 0.5f);
                    g.speed = (dist(rng) < 0.1f) ? 4.0f : 1.0f; break;
                case 8: // Arp
                    {
                        float notes[] = { 1.0f, 1.25f, 1.5f, 1.875f, 2.0f }; // Major scale steps
                        g.speed = notes[stepCount % 5];
                    } break;
                case 9: // Pattern
                    g.speed = 1.0f; break;
                case 10: // Warp
                    g.speed = 1.0f + std::sin((float)stepCount * 0.5f) * shape; break;
            }

            // Length logic
            switch (algo) {
                case 0: g.length = timeMs * (0.2f + shape * 0.8f) * ((float)fs / 1000.0f); break;
                case 3: case 4: g.length = (100.0f + dist(rng) * 1000.0f * shape) * ((float)fs / 1000.0f); break;
                case 9: g.length = (timeMs * 0.5f) * ((float)fs / 1000.0f); break;
                default: g.length = (timeMs * 0.25f) * ((float)fs / 1000.0f); break;
            }
            if (g.length < 50.0f) g.length = 50.0f;

            // Positioning
            float offset = (0.01f + dist(rng) * 0.99f) * (float)fs * (0.1f + activity * 2.0f);
            if (algo == 9) offset = (float)(stepCount % 4) * samplesPerStep; 

            g.currentPos = (float)currentWriteIdx - offset;
            while (g.currentPos < 0) g.currentPos += (float)maxDelaySamples;
            while (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;

            g.pan = dist(rng);
            
            // Assign window type based on shape knob (honoring the UI labels)
            if (shape < 0.2f) g.windowType = 0; // Sine
            else if (shape < 0.4f) g.windowType = 1; // Tri
            else if (shape < 0.6f) g.windowType = 2; // Saw
            else if (shape < 0.8f) g.windowType = 3; // Square
            else g.windowType = (int)(dist(rng) * 4.0f); // Random
            
            break;
        }
    }
}

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts) {
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Sync Targets
    smoothActivity.setTargetValue(apvts.getRawParameterValue("ACTIVITY")->load());
    smoothTime.setTargetValue(apvts.getRawParameterValue("TIME")->load());
    smoothShape.setTargetValue(apvts.getRawParameterValue("SHAPE")->load());
    smoothRepeats.setTargetValue(apvts.getRawParameterValue("REPEATS")->load());
    smoothFilter.setTargetValue(apvts.getRawParameterValue("FILTER")->load());
    smoothSpace.setTargetValue(apvts.getRawParameterValue("SPACE")->load());
    smoothMix.setTargetValue(apvts.getRawParameterValue("MIX")->load());
    smoothLoopLevel.setTargetValue(apvts.getRawParameterValue("LOOP_LEVEL")->load());
    smoothGain.setTargetValue(apvts.getRawParameterValue("GAIN")->load());

    int algo = (int)apvts.getRawParameterValue("ALGO")->load();
    bool tempoMode = apvts.getRawParameterValue("TEMPO_MODE")->load() > 0.5f;
    float resonance = apvts.getRawParameterValue("RESONANCE")->load();

    juce::AudioBuffer<float> wetBuffer(numChannels, numSamples);
    wetBuffer.clear();

    // Process Reverb/Filter Parameters Once Per Block
    float filterKnob = smoothFilter.getNextValue(); // Grab start-of-block value
    auto& filter = fxChain.get<0>();
    filter.setResonance(juce::jlimit(0.0f, 0.90f, resonance * 0.85f));
    
    if (filterKnob < 0.45f) {
        if (currentFilterMode != juce::dsp::LadderFilterMode::LPF24) {
            currentFilterMode = juce::dsp::LadderFilterMode::LPF24;
            filter.setMode(currentFilterMode);
            filter.reset(); // Prevent pops
        }
        float logCutoff = juce::mapToLog10(juce::jlimit(0.001f, 1.0f, filterKnob / 0.45f), 40.0f, 20000.0f);
        filter.setCutoffFrequencyHz(logCutoff);
    } else if (filterKnob > 0.55f) {
        if (currentFilterMode != juce::dsp::LadderFilterMode::HPF24) {
            currentFilterMode = juce::dsp::LadderFilterMode::HPF24;
            filter.setMode(currentFilterMode);
            filter.reset(); // Prevent pops
        }
        float logCutoff = juce::mapToLog10(juce::jlimit(0.001f, 1.0f, (filterKnob - 0.55f) / 0.45f), 20.0f, 18000.0f);
        filter.setCutoffFrequencyHz(logCutoff);
    } else {
        if (currentFilterMode != juce::dsp::LadderFilterMode::LPF24) {
            currentFilterMode = juce::dsp::LadderFilterMode::LPF24;
            filter.setMode(currentFilterMode);
            filter.reset();
        }
        filter.setCutoffFrequencyHz(20000.0f); // Basically bypassed
    }

    float spaceVal = smoothSpace.getNextValue();
    fxChain.get<1>().setParameters({spaceVal * 0.8f, 0.5f, spaceVal * 0.5f, 1.0f, 1.0f, 0.0f});
    
    // Fast-forward the block-level smoothers so they reach target
    smoothFilter.skip(numSamples - 1);
    smoothSpace.skip(numSamples - 1);

    for (int i = 0; i < numSamples; ++i) {
        float timeVal = smoothTime.getNextValue();
        float effectiveTimeMs = 250.0f;
        if (tempoMode) {
            float bpm = 40.0f + (timeVal * 200.0f);
            effectiveTimeMs = 60000.0f / bpm; 
        } else {
            int subdiv = (int)apvts.getRawParameterValue("SUBDIV")->load();
            float factors[] = { 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
            effectiveTimeMs = 1000.0f * factors[juce::jlimit(0, 4, subdiv)]; 
        }

        int currentWriteIdx = (writeIdx + i) % maxDelaySamples;
        scheduleGrains(smoothActivity.getNextValue(), effectiveTimeMs, smoothShape.getNextValue(), algo, currentWriteIdx, apvts);
        
        for (auto& g : grains) {
            if (g.active) {
                float phase = g.age / g.length;
                float window = 0.0f;

                // Dynamic Windowing (Envelope Shaping)
                switch (g.windowType) {
                    case 0: // Sine (Hann)
                        window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase)); break;
                    case 1: // Triangle
                        window = 1.0f - std::abs(2.0f * phase - 1.0f); break;
                    case 2: // Saw
                        window = 1.0f - phase; break;
                    case 3: // Square-ish (Trapezoid)
                        window = phase < 0.1f ? (phase * 10.0f) : (phase > 0.9f ? (1.0f - phase) * 10.0f : 1.0f); break;
                    default: // Random selection
                        window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase)); break;
                }
                
                float sL = getNextSample(0, g.currentPos) * window;
                float sR = getNextSample(numChannels > 1 ? 1 : 0, g.currentPos) * window;
                
                wetBuffer.addSample(0, i, sL * (1.0f - g.pan) * 0.8f);
                if (numChannels > 1) wetBuffer.addSample(1, i, sR * g.pan * 0.8f);
                
                float actualSpeed = g.reverse ? -g.speed : g.speed;
                g.currentPos += actualSpeed;
                if (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;
                if (g.currentPos < 0) g.currentPos += (float)maxDelaySamples;

                if (++g.age >= g.length) g.active = false;
            }
        }
    }

    // Process FX chain on the wet signal
    juce::dsp::AudioBlock<float> block(wetBuffer);
    fxChain.process(juce::dsp::ProcessContextReplacing<float>(block));

    // Feedback loop and Mixing
    for (int i = 0; i < numSamples; ++i) {
        float mix = smoothMix.getNextValue();
        float gain = smoothGain.getNextValue();
        float repeats = smoothRepeats.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch) {
            float dry = buffer.getSample(ch, i);
            float wet = wetBuffer.getSample(ch, i);
            
            if (!std::isfinite(wet)) wet = 0.0f;
            wet = juce::jlimit(-2.0f, 2.0f, wet); // Hard safety clipper

            // Correct 1-pole Highpass DC Blocker for feedback
            float prevX = (ch == 0) ? dcBlockerX_L : dcBlockerX_R;
            float prevY = (ch == 0) ? dcBlockerY_L : dcBlockerY_R;
            float dcBlockedWet = wet - prevX + 0.995f * prevY;
            
            if (ch == 0) { dcBlockerX_L = wet; dcBlockerY_L = dcBlockedWet; }
            else         { dcBlockerX_R = wet; dcBlockerY_R = dcBlockedWet; }
            
            // Limit feedback to prevent total explosion
            float fbSignal = juce::jlimit(-1.5f, 1.5f, dcBlockedWet * repeats * 0.8f);

            // Update delay buffer with input + safe feedback
            float writeVal = dry + fbSignal;
            delayBuffer.setSample(ch, writeIdx, std::tanh(writeVal));

            // Final Output Mix
            float out = (dry * (1.0f - mix)) + (wet * mix);
            buffer.setSample(ch, i, std::tanh(out * gain));
        }
        writeIdx = (writeIdx + 1) % maxDelaySamples;
    }
}
