/**
 * COSMIC RAYS - Granular Engine Implementation (Definitive Stability Fix)
 * ---------------------------------------------------------------------
 * Robust granular synthesis with proper feedback path and direction logic.
 */

#include "GranularEngine.h"
#include <cmath>

GranularEngine::GranularEngine() : rng(std::random_device{}()), dist(0.0f, 1.0f) {
    grains.resize(64); 
}

void GranularEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels) {
    fs = sampleRate;
    preparedChannels = numChannels;
    
    maxDelaySamples = static_cast<int>(sampleRate * 2.0); 
    maxHoldSamples = static_cast<int>(sampleRate * 1.0);  
    delayBuffer.setSize(numChannels, maxDelaySamples);
    holdBuffer.setSize(numChannels, maxHoldSamples);
    loopBuffer.setSize(numChannels, (int)(sampleRate * 60.0)); 
    mdlBuffer.setSize(numChannels, 16384); 
    
    delayBuffer.clear(); holdBuffer.clear(); loopBuffer.clear(); mdlBuffer.clear();
    writeIdx = 0; holdWriteIdx = 0; mdlWriteIdx = 0; mdlPhase = 0.0f; mdlPhase2 = 0.0f; mdlPhase3 = 0.0f;
    holdLocked = false;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };
    fxChain.prepare(spec);
    
    diffusers.clear(); diffuserPhases.assign(numDiffusers, 0.0f);
    for (int i = 0; i < numDiffusers; ++i) {
        auto d = std::make_unique<juce::dsp::DelayLine<float>>(8000);
        d->prepare(spec); d->setDelay((15.0f + (float)i * 25.0f) / 1000.0f * (float)sampleRate);
        diffusers.push_back(std::move(d)); diffuserPhases[i] = dist(rng);
    }

    auto tiltCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 100.0f, 0.707f, 1.25f);
    tiltFilterL.coefficients = tiltFilterR.coefficients = tiltCoeffs;
    tiltFilterL.prepare(spec); tiltFilterR.prepare(spec);
    limiter.prepare(spec); limiter.setThreshold(-0.1f); limiter.setRelease(50.0f);
    
    currentFilterMode = juce::dsp::LadderFilterMode::LPF12; 
    fxChain.get<0>().setMode(currentFilterMode);
    fxChain.get<2>().setMix(0.0f); 

    for (auto* s : {&smoothActivity, &smoothTime, &smoothShape, &smoothRepeats, &smoothFilter, &smoothSpace, &smoothMix, &smoothGain, &smoothLoopLevel, &smoothDrift, &smoothSnap, &smoothMasterWetVol, &smoothLoopFade, &smoothSpray, &smoothSpread, &smoothEnvFollower, &smoothPitchJitter, &smoothRevProb, &smoothModRate, &smoothModDepth}) {
        s->reset(sampleRate, 0.05);
    }
    smoothEnvFollower.reset(sampleRate, 0.01);
    smoothMasterWetVol.setTargetValue(1.0f);
    smoothLoopFade.setTargetValue(1.0f);

    grainFilters.assign(64, std::vector<FilterState>(numChannels));
    currentActiveGrainCount = 0.0f; interruptGate = 1.0f; interruptTimer = 0;
    for (auto& g : grains) g.active = false;
}

float GranularEngine::getNextSampleCubic(int channel, float readPos) {
    if (channel >= delayBuffer.getNumChannels()) channel = 0;
    int i = (int)std::floor(readPos);
    float f = readPos - (float)i;
    auto get = [&](int idx) {
        int pos = idx % maxDelaySamples;
        if (pos < 0) pos += maxDelaySamples;
        return delayBuffer.getSample(channel, pos);
    };
    float y0 = get(i - 1), y1 = get(i), y2 = get(i + 1), y3 = get(i + 2);
    float a0 = -0.5f*y0 + 1.5f*y1 - 1.5f*y2 + 0.5f*y3, a1 = y0 - 2.5f*y1 + 2.0f*y2 - 0.5f*y3, a2 = -0.5f*y0 + 0.5f*y2, a3 = y1;
    float out = a0*f*f*f + a1*f*f + a2*f + a3;
    return std::isfinite(out) ? out : 0.0f;
}

void GranularEngine::updateEnvelopeFollower(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f; 
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) 
        peak = std::max(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    float alpha = peak > envFollower ? 0.05f : 0.001f; 
    envFollower = alpha * peak + (1.0f - alpha) * envFollower;
    static float prevEnv = 0.0f; 
    transientDetected = (envFollower > 0.05f && (envFollower - prevEnv) > 0.02f); 
    prevEnv = envFollower;
}

void GranularEngine::scheduleGrains(float activity, float timeMs, float shape, int algo, int currentWriteIdx, juce::AudioProcessorValueTreeState& apvts) {
    bool globalRev = apvts.getRawParameterValue("LOOPER_REV")->load() > 0.5f;
    bool globalQuant = apvts.getRawParameterValue("LOOPER_QUANT")->load() > 0.5f;
    float repeats = apvts.getRawParameterValue("REPEATS")->load();
    float spray = apvts.getRawParameterValue("SPRAY")->load();
    float spread = apvts.getRawParameterValue("SPREAD")->load();
    float jitter = apvts.getRawParameterValue("PITCH_JITTER")->load();
    float revProb = apvts.getRawParameterValue("REV_PROB")->load();
    int globalWin = (int)apvts.getRawParameterValue("WINDOW_TYPE")->load();

    float samplesPerStep = (timeMs * (float)fs / 1000.0f) * 0.25f;
    if (samplesPerStep < 100.0f) samplesPerStep = 100.0f;
    grainClock += 1.0f; bool isStep = false; 
    if (grainClock >= samplesPerStep) { grainClock -= samplesPerStep; stepCount = (stepCount + 1) % 16; isStep = true; }
    
    bool trigger = false; int mode = (int)(shape * 3.99f); 
    if (algo == 7) { 
        float prob = 0.001f + (activity * 0.15f); 
        if (transientDetected) prob += (activity * 0.3f); 
        trigger = (dist(rng) < prob); 
        if (trigger && interruptTimer <= 0) interruptTimer = (int)(repeats * 1.5f * fs + 0.1f * fs);
    }
    else if (globalQuant) trigger = isStep && (dist(rng) < activity + 0.15f);
    else { 
        float prob = 0.005f + (activity * 0.1f); 
        trigger = (dist(rng) < prob); 
    }

    if (!trigger && algo != 7) return; 
    
    bool spawnInterruptGrain = (algo == 7 && interruptTimer > 0 && dist(rng) < 0.01f);
    if (!trigger && !spawnInterruptGrain) return;

    int spawnCount = (algo == 0 || algo == 3) ? (int)(1 + activity * 12.0f) : 1;
    for (int s = 0; s < spawnCount; ++s) {
        for (int i = 0; i < (int)grains.size(); ++i) {
            auto& g = grains[i];
            if (!g.active) {
                g.active = true; g.age = 0.0f; g.startSpeed = 1.0f; g.endSpeed = 1.0f;
                g.internalPhase = dist(rng); 
                g.reverse = (dist(rng) < revProb) ? !globalRev : globalRev; 
                g.filterCutoff = 1.0f; g.bitCrush = 1.0f; g.isSustainer = false;
                
                float lenBase = timeMs * ((float)fs / 1000.0f) * (1.0f + activity * 1.5f);
                float random_spread = (dist(rng) * 2.0f - 1.0f) * activity * 1000.0f;
                float offset = dist(rng) * (float)fs * (0.1f + activity * 1.0f) + random_spread;
                
                offset += (dist(rng) * 2.0f - 1.0f) * spray * (float)fs * 0.5f;
                g.pan = juce::jlimit(0.0f, 1.0f, 0.5f + (dist(rng) - 0.5f) * (spread + activity * 0.2f));

                if (algo == 0) { 
                    if (mode == 0) g.startSpeed = (dist(rng) < 0.5f) ? 1.0f : 2.0f; 
                    else if (mode == 1) g.startSpeed = (dist(rng) < 0.5f) ? 1.0f : 0.5f; 
                    else if (mode == 2) g.startSpeed = 2.0f; 
                    else { float ss[]={0.5f, 1.0f, 2.0f, 4.0f}; g.startSpeed = ss[(int)(dist(rng)*4.0f)]; } 
                    g.endSpeed = g.startSpeed; g.length = lenBase * (0.5f + repeats * 3.0f); 
                } 
                else if (algo == 1) { 
                    g.length = lenBase * (0.25f + repeats * 2.0f); 
                    if (mode == 0) { g.filterCutoff = 0.2f + (1.0f - activity) * 0.5f; offset = (float)(dist(rng) * samplesPerStep * 8.0f); } 
                    else if (mode == 1) { g.startSpeed = (dist(rng) < activity) ? 0.5f : 1.0f; g.endSpeed = g.startSpeed; } 
                }
                else if (algo == 2) { 
                    g.length = lenBase * (0.5f + repeats * 10.0f); 
                    float depthSemi = (mode == 1) ? 12.0f : 6.0f;
                    g.startSpeed = (mode == 0) ? 0.75f : (mode == 2 ? 1.5f : 1.0f); 
                    g.endSpeed = depthSemi; 
                    g.filterCutoff = 0.85f - (activity * 0.3f);
                }
                
                g.length = std::max(100.0f, std::min(g.length, (float)maxDelaySamples - 100.0f));

                if (jitter > 0.05f) {
                    float jOff = (dist(rng) * 2.0f - 1.0f) * jitter; 
                    float multiplier = std::pow(2.0f, jOff / 12.0f);
                    g.startSpeed *= multiplier;
                }
                
                offset += 0.0075f * (float)fs; 
                g.currentPos = (float)currentWriteIdx - offset; 
                while (g.currentPos < 0) g.currentPos += (float)maxDelaySamples; 
                while (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;
                g.startPos = g.currentPos;
                g.windowType = (globalWin > 0) ? globalWin - 1 : mode; 
                for (auto& f : grainFilters[i]) { f.x1 = 0; f.y1 = 0; } break;
            }
        }
    }
}

void GranularEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts) {
    int numSamples = buffer.getNumSamples(); int numChannels = buffer.getNumChannels();
    updateEnvelopeFollower(buffer);
    
    smoothActivity.setTargetValue(apvts.getRawParameterValue("ACTIVITY")->load());
    smoothTime.setTargetValue(apvts.getRawParameterValue("TIME")->load());
    smoothShape.setTargetValue(apvts.getRawParameterValue("SHAPE")->load());
    smoothRepeats.setTargetValue(apvts.getRawParameterValue("REPEATS")->load());
    smoothFilter.setTargetValue(apvts.getRawParameterValue("FILTER")->load());
    smoothSpace.setTargetValue(apvts.getRawParameterValue("SPACE")->load());
    smoothMix.setTargetValue(apvts.getRawParameterValue("MIX")->load());
    smoothGain.setTargetValue(apvts.getRawParameterValue("GAIN")->load());
    smoothMasterWetVol.setTargetValue(apvts.getRawParameterValue("MASTER_WET_VOL")->load());
    smoothLoopFade.setTargetValue(apvts.getRawParameterValue("LOOP_FADE")->load());
    smoothLoopLevel.setTargetValue(apvts.getRawParameterValue("LOOP_LEVEL")->load());
    smoothSpray.setTargetValue(apvts.getRawParameterValue("SPRAY")->load());
    smoothSpread.setTargetValue(apvts.getRawParameterValue("SPREAD")->load());
    smoothPitchJitter.setTargetValue(apvts.getRawParameterValue("PITCH_JITTER")->load());
    smoothRevProb.setTargetValue(apvts.getRawParameterValue("REV_PROB")->load());
    smoothModRate.setTargetValue(apvts.getRawParameterValue("MOD_RATE")->load());
    smoothModDepth.setTargetValue(apvts.getRawParameterValue("MOD_DEPTH")->load());
    
    int algo = (int)apvts.getRawParameterValue("ALGO")->load();
    bool freeze = apvts.getRawParameterValue("FREEZE")->load() > 0.5f;
    
    juce::AudioBuffer<float> wetBuffer(numChannels, numSamples); wetBuffer.clear();
    
    for (int i = 0; i < numSamples; ++i) {
        float timeVal = smoothTime.getNextValue(), effectiveTimeMs = 50.0f + (timeVal * 1950.0f);
        float activityVal = smoothActivity.getNextValue();
        float modRateVal = smoothModRate.getNextValue();
        float modDepthVal = smoothModDepth.getNextValue();
        
        float primaryRate = 0.01f + modRateVal * 1.99f;
        mdlPhase += (juce::MathConstants<float>::twoPi * primaryRate) / fs;
        mdlPhase2 += (juce::MathConstants<float>::twoPi * 0.073f) / fs; 
        if (mdlPhase >= juce::MathConstants<float>::twoPi) mdlPhase -= juce::MathConstants<float>::twoPi;
        if (mdlPhase2 >= juce::MathConstants<float>::twoPi) mdlPhase2 -= juce::MathConstants<float>::twoPi;
        
        float compoundLfo = (std::sin(mdlPhase) + 0.4f * std::sin(mdlPhase * 2.0f + 0.5f)) * (0.7f + 0.3f * std::sin(mdlPhase2));
        float depthSamples = (modDepthVal * 50.0f / 1000.0f) * fs;
        float currentMdlDelay = (depthSamples + 10.0f) + (compoundLfo * depthSamples);

        scheduleGrains(activityVal, effectiveTimeMs, smoothShape.getNextValue(), algo, writeIdx, apvts);
        
        if (algo == 7) { 
            if (interruptTimer > 0) { interruptGate = juce::jmax(0.0f, interruptGate - 0.01f); interruptTimer--; } 
            else { interruptGate = juce::jmin(1.0f, interruptGate + 0.005f); } 
        } else { interruptGate = 1.0f; interruptTimer = 0; }

        for (int gIdx = 0; gIdx < (int)grains.size(); ++gIdx) {
            auto& g = grains[gIdx];
            if (g.active) {
                float phase = g.age / g.length;
                float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase)); 
                
                float sL = getNextSampleCubic(0, g.currentPos) * window;
                float sR = getNextSampleCubic(numChannels > 1 ? 1 : 0, g.currentPos) * window;
                
                wetBuffer.addSample(0, i, sL * (1.0f - g.pan)); 
                if (numChannels > 1) wetBuffer.addSample(1, i, sR * g.pan);
                
                if (algo == 2) { 
                    float triPhase = (g.age / fs) * (0.05f + activityVal * 4.0f) + g.internalPhase;
                    float tri = 2.0f * std::abs(2.0f * (triPhase - std::floor(triPhase + 0.5f))) - 1.0f;
                    float speedMultiplier = std::pow(2.0f, (tri * g.endSpeed) / 12.0f);
                    float instSpeed = (g.reverse ? -g.startSpeed : g.startSpeed) * speedMultiplier;
                    g.currentPos += instSpeed;
                } else {
                    g.currentPos += g.reverse ? -g.startSpeed : g.startSpeed;
                }
                
                while (g.currentPos < 0) g.currentPos += (float)maxDelaySamples;
                while (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;

                if (++g.age >= g.length) g.active = false;
            }
        }

        float mix = smoothMix.getNextValue(), gain = smoothGain.getNextValue();
        float masterWet = smoothMasterWetVol.getNextValue();
        float repeats = smoothRepeats.getNextValue();
        int activeChannels = std::min(numChannels, preparedChannels);

        for (int ch = 0; ch < activeChannels; ++ch) {
            float dryInput = buffer.getSample(ch, i);
            float rawWet = wetBuffer.getSample(ch, i) * masterWet;
            
            mdlBuffer.setSample(ch, mdlWriteIdx, rawWet);
            float mdlReadPos = (float)mdlWriteIdx - currentMdlDelay;
            while (mdlReadPos < 0) mdlReadPos += 16384.0f;
            while (mdlReadPos >= 16384.0f) mdlReadPos -= 16384.0f;
            
            int i0 = (int)std::floor(mdlReadPos);
            float f = mdlReadPos - (float)i0;
            auto getMdl = [&](int idx) { int p = idx % 16384; if (p < 0) p += 16384; return mdlBuffer.getSample(ch, p); };
            float m0 = getMdl(i0 - 1), m1 = getMdl(i0), m2 = getMdl(i0 + 1), m3 = getMdl(i0 + 2);
            float a0 = -0.5f*m0 + 1.5f*m1 - 1.5f*m2 + 0.5f*m3, a1 = m0 - 2.5f*m1 + 2.0f*m2 - 0.5f*m3, a2 = -0.5f*m0 + 0.5f*m2, a3 = m1;
            float wet = a0*f*f*f + a1*f*f + a2*f + a3;
            if (!std::isfinite(wet)) wet = 0.0f;
            
            buffer.setSample(ch, i, (dryInput * (1.0f - mix) + wet * mix) * gain);
            
            if (!freeze) {
                float fb = wet * repeats * 0.8f;
                delayBuffer.setSample(ch, writeIdx, std::tanh(dryInput + fb));
            }
        }
        mdlWriteIdx = (mdlWriteIdx + 1) % 16384;
        if (!freeze) writeIdx = (writeIdx + 1) % maxDelaySamples;
    }
    
    for (int ch = 0; ch < numChannels; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int s = 0; s < numSamples; ++s) if (!std::isfinite(data[s])) data[s] = 0.0f;
    }
    juce::dsp::AudioBlock<float> outputBlock(buffer); 
    limiter.process(juce::dsp::ProcessContextReplacing<float>(outputBlock));
}
