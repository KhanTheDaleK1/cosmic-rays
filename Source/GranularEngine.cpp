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
    
    currentFilterMode = juce::dsp::LadderFilterMode::LPF12;
    fxChain.get<0>().setMode(currentFilterMode);
    
    // Reverb for Hall character
    auto& reverb = fxChain.get<1>();
    juce::dsp::Reverb::Parameters revParams;
    revParams.roomSize = 0.85f;
    revParams.damping = 0.4f;
    revParams.width = 1.0f;
    revParams.freezeMode = 0.0f;
    reverb.setParameters(revParams);

    smoothActivity.reset(sampleRate, 0.05);
    smoothTime.reset(sampleRate, 0.05);
    smoothShape.reset(sampleRate, 0.05);
    smoothRepeats.reset(sampleRate, 0.05);
    smoothFilter.reset(sampleRate, 0.05);
    smoothSpace.reset(sampleRate, 0.05);
    smoothMix.reset(sampleRate, 0.05);
    smoothGain.reset(sampleRate, 0.05);
    smoothLoopLevel.reset(sampleRate, 0.05);
    smoothDrift.reset(sampleRate, 0.05);
    smoothSnap.reset(sampleRate, 0.05);

    loopBuffer.setSize(numChannels, (int)(sampleRate * 60.0)); // 60 second looper
    loopBuffer.clear();
    loopWritePos = 0; loopReadPos = 0; loopLength = 0;
    isOverdubbing = false;

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
    float drift = apvts.getRawParameterValue("PITCH_DRIFT")->load();
    float snap = apvts.getRawParameterValue("SNAP_STRENGTH")->load();

    float samplesPerStep = (timeMs * (float)fs / 1000.0f) * 0.25f; 
    if (samplesPerStep < 100.0f) samplesPerStep = 100.0f;
    
    grainClock += 1.0f;
    bool isStep = false;
    if (grainClock >= samplesPerStep) {
        grainClock -= samplesPerStep;
        stepCount = (stepCount + 1) % 16;
        isStep = true;
    }

    bool trigger = false;
    float prob = 0.002f + (activity * 0.08f);

    if (globalQuant || (dist(rng) < snap)) {
        trigger = isStep && (dist(rng) < activity + 0.15f);
    } else {
        switch (algo) {
            case 0: case 3: case 4: trigger = (dist(rng) < prob); break;
            default: trigger = isStep && (dist(rng) < activity + 0.05f); break;
        }
    }

    if (!trigger) return;

    int spawnCount = (algo == 0 || algo == 3) ? (int)(1 + activity * 2.0f) : 1;
    
    for (int s = 0; s < spawnCount; ++s) {
        for (auto& g : grains) {
            if (!g.active) {
                g.active = true; g.age = 0.0f; 
                g.startSpeed = 1.0f; g.endSpeed = 1.0f;
                g.reverse = globalRev;

                if (algo == 0) { // Mosaic
                    float octs[] = { 0.5f, 1.0f, 2.0f, 1.5f };
                    g.startSpeed = octs[(int)(dist(rng) * 4.0f)];
                    g.endSpeed = g.startSpeed;
                } else if (algo == 1) { // Seq
                    g.startSpeed = (stepCount % 4 == 0) ? 2.0f : (stepCount % 3 == 0 ? 0.5f : 1.0f);
                    g.endSpeed = g.startSpeed;
                } else if (algo == 2) { // Glide
                    g.startSpeed = 1.0f; g.endSpeed = 0.5f + (shape * 1.5f);
                } else if (algo == 3) { // Haze
                    g.startSpeed = 1.0f + (dist(rng) * 2.0f - 1.0f) * shape * 0.5f;
                    g.endSpeed = g.startSpeed;
                } else if (algo == 4) { // Tunnel
                    g.startSpeed = 0.5f; g.endSpeed = 0.5f;
                } else if (algo == 5) { // Strum
                    g.startSpeed = 1.0f + (float)(stepCount % 6) * 0.2f;
                    g.endSpeed = g.startSpeed;
                } else if (algo == 6) { // Blocks
                    g.reverse = (stepCount % 4 == 0) ? !globalRev : globalRev;
                } else if (algo == 7) { // Interrupt
                    if (dist(rng) < activity) g.startSpeed = (dist(rng) < 0.5f) ? 4.0f : 0.25f;
                    g.endSpeed = g.startSpeed;
                } else if (algo == 8) { // Arp
                    float notes[] = { 1.0f, 1.25f, 1.5f, 1.875f, 2.0f };
                    g.startSpeed = notes[stepCount % 5];
                    g.endSpeed = g.startSpeed;
                } else if (algo == 9) { // Pattern
                    g.startSpeed = 1.0f; g.endSpeed = 1.0f;
                } else if (algo == 10) { // Warp
                    g.startSpeed = 1.0f + std::sin((float)stepCount * 0.8f) * shape;
                    g.endSpeed = 1.0f - std::sin((float)stepCount * 0.8f) * shape;
                }

                if (drift > 0.05f) {
                    float d = (dist(rng) * 2.0f - 1.0f) * drift * 0.2f;
                    g.startSpeed += d; g.endSpeed += d;
                }

                float lenBase = timeMs * ((float)fs / 1000.0f);
                if (algo >= 3 && algo <= 5) g.length = lenBase * (1.0f + shape * 2.0f);
                else if (algo >= 9) g.length = lenBase * 0.5f;
                else g.length = lenBase * (0.2f + shape * 0.8f);

                if (g.length < 100.0f) g.length = 100.0f;

                float maxOffset = (float)fs * (0.2f + activity * 3.0f);
                float offset = dist(rng) * maxOffset;
                if (algo == 4) offset = (float)fs * 0.5f;
                else if (algo == 9) offset = (float)(stepCount % 4) * samplesPerStep * 2.0f;

                g.currentPos = (float)currentWriteIdx - offset;
                while (g.currentPos < 0) g.currentPos += (float)maxDelaySamples;
                while (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;

                g.pan = dist(rng);
                if (shape < 0.25f) g.windowType = 0;
                else if (shape < 0.5f) g.windowType = 1;
                else if (shape < 0.75f) g.windowType = 2;
                else g.windowType = 3;
                break;
            }
        }
    }
}

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts) {
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

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
    bool freeze = apvts.getRawParameterValue("FREEZE")->load() > 0.5f;
    bool killDry = apvts.getRawParameterValue("KILL_DRY")->load() > 0.5f;
    bool isLooperPost = ((int)apvts.getRawParameterValue("LOOPER_MODE")->load()) == 1;
    bool isRecording = apvts.getRawParameterValue("LOOPER_REC")->load() > 0.5f;
    bool isOverdubbing = apvts.getRawParameterValue("LOOPER_ODUB")->load() > 0.5f;

    float modRate = apvts.getRawParameterValue("MOD_RATE")->load();
    float modDepth = apvts.getRawParameterValue("MOD_DEPTH")->load();

    juce::AudioBuffer<float> wetBuffer(numChannels, numSamples);
    wetBuffer.clear();

    // FX Param Update
    float filterKnob = smoothFilter.getNextValue(); 
    auto& filter = fxChain.get<0>();
    filter.setResonance(juce::jlimit(0.0f, 0.90f, (float)apvts.getRawParameterValue("RESONANCE")->load() * 0.85f));
    
    if (filterKnob < 0.45f) {
        if (currentFilterMode != juce::dsp::LadderFilterMode::LPF12) {
            currentFilterMode = juce::dsp::LadderFilterMode::LPF12;
            filter.setMode(currentFilterMode); filter.reset();
        }
        filter.setCutoffFrequencyHz(juce::mapToLog10(juce::jlimit(0.001f, 1.0f, filterKnob / 0.45f), 40.0f, 20000.0f));
    } else if (filterKnob > 0.55f) {
        if (currentFilterMode != juce::dsp::LadderFilterMode::HPF12) {
            currentFilterMode = juce::dsp::LadderFilterMode::HPF12;
            filter.setMode(currentFilterMode); filter.reset();
        }
        filter.setCutoffFrequencyHz(juce::mapToLog10(juce::jlimit(0.001f, 1.0f, (filterKnob - 0.55f) / 0.45f), 20.0f, 18000.0f));
    } else {
        filter.setCutoffFrequencyHz(20000.0f);
    }

    float spaceVal = smoothSpace.getNextValue();
    fxChain.get<1>().setParameters({spaceVal * 0.9f, 0.4f, spaceVal * 0.6f, 1.0f, 1.0f, 0.0f});

    for (int i = 0; i < numSamples; ++i) {
        float timeVal = smoothTime.getNextValue();
        float effectiveTimeMs = 250.0f;
        if (tempoMode) effectiveTimeMs = 60000.0f / (40.0f + (timeVal * 200.0f));
        else {
            float factors[] = { 1.0f, 0.5f, 0.25f, 0.125f, 0.0625f };
            effectiveTimeMs = 1000.0f * factors[juce::jlimit(0, 4, (int)apvts.getRawParameterValue("SUBDIV")->load())];
        }

        // Wow & Flutter modulation
        modPhase += (modRate / (float)fs);
        if (modPhase >= 1.0f) modPhase -= 1.0f;
        float modLFO = std::sin(juce::MathConstants<float>::twoPi * modPhase) * modDepth * 0.02f;

        scheduleGrains(smoothActivity.getNextValue(), effectiveTimeMs, smoothShape.getNextValue(), algo, writeIdx, apvts);
        
        for (auto& g : grains) {
            if (g.active) {
                float phase = g.age / g.length;
                float window = 0.0f;
                switch (g.windowType) {
                    case 0: window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase)); break;
                    case 1: window = 1.0f - std::abs(2.0f * phase - 1.0f); break;
                    case 2: window = 1.0f - phase; break;
                    case 3: window = phase < 0.05f ? (phase * 20.0f) : (phase > 0.95f ? (1.0f - phase) * 20.0f : 1.0f); break;
                }
                
                float sL = getNextSample(0, g.currentPos) * window;
                float sR = getNextSample(numChannels > 1 ? 1 : 0, g.currentPos) * window;
                
                wetBuffer.addSample(0, i, sL * (1.0f - g.pan));
                if (numChannels > 1) wetBuffer.addSample(1, i, sR * g.pan);
                
                float curSpeed = (g.startSpeed + (g.endSpeed - g.startSpeed) * phase) + modLFO;
                float actualSpeed = g.reverse ? -curSpeed : curSpeed;
                g.currentPos += actualSpeed;
                if (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;
                if (g.currentPos < 0) g.currentPos += (float)maxDelaySamples;
                if (++g.age >= g.length) g.active = false;
            }
        }
    }

    juce::dsp::AudioBlock<float> wetBlock(wetBuffer);
    fxChain.process(juce::dsp::ProcessContextReplacing<float>(wetBlock));

    for (int i = 0; i < numSamples; ++i) {
        float mix = smoothMix.getNextValue(), gain = smoothGain.getNextValue(), repeats = smoothRepeats.getNextValue();
        float loopLevel = smoothLoopLevel.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch) {
            float dry = buffer.getSample(ch, i);
            float wet = wetBuffer.getSample(ch, i);
            
            float loopOut = (loopLength > 0) ? loopBuffer.getSample(ch, loopReadPos) : 0.0f;
            float looperInput = isLooperPost ? (dry * (1.0f - mix) + wet * mix) : dry;
            
            if (isRecording) loopBuffer.setSample(ch, loopWritePos, looperInput);
            else if (isOverdubbing) loopBuffer.setSample(ch, loopWritePos, loopBuffer.getSample(ch, loopWritePos) + looperInput * 0.6f);

            float prevX = (ch == 0) ? dcBlockerX_L : dcBlockerX_R;
            float prevY = (ch == 0) ? dcBlockerY_L : dcBlockerY_R;
            float dcBlockedWet = wet - prevX + 0.995f * prevY;
            if (ch == 0) { dcBlockerX_L = wet; dcBlockerY_L = dcBlockedWet; }
            else         { dcBlockerX_R = wet; dcBlockerY_R = dcBlockedWet; }
            
            float fbSignal = juce::jlimit(-1.5f, 1.5f, dcBlockedWet * repeats * 0.85f);
            if (!freeze) delayBuffer.setSample(ch, writeIdx, std::tanh(dry + fbSignal));

            float finalWet = wet + (loopOut * loopLevel);
            float out = ((killDry ? 0.0f : dry) * (1.0f - mix)) + (finalWet * mix);
            buffer.setSample(ch, i, std::tanh(out * gain));
        }
        
        loopReadPos = (loopReadPos + 1) % juce::jmax(1, loopLength);
        if (isRecording) {
            loopWritePos++;
            if (loopWritePos >= loopBuffer.getNumSamples()) loopWritePos = 0;
            loopLength = juce::jmax(loopLength, loopWritePos);
        } else loopWritePos = (loopWritePos + 1) % juce::jmax(1, loopLength);

        if (!freeze) writeIdx = (writeIdx + 1) % maxDelaySamples;
    }
}
