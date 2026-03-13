/**
 * COSMIC RAYS - Granular Engine Implementation (Definitive Stability Fix)
 * ---------------------------------------------------------------------
 * Robust granular synthesis with proper feedback path and direction logic.
 */

#include "GranularEngine.h"
#include <cmath>

GranularEngine::GranularEngine() : rng(std::random_device{}()), dist(0.0f, 1.0f) {
    grains.resize(256); 
}

void GranularEngine::prepare(double sampleRate, int samplesPerBlock, int numChannels, juce::AudioProcessorValueTreeState& apvts) {
    fs = sampleRate;
    preparedChannels = std::max(1, numChannels);
    wetBuffer.setSize(preparedChannels, samplesPerBlock, false, false, true);
    wetBuffer.clear();
    
    maxDelaySamples = static_cast<int>(sampleRate * 2.0); 
    maxHoldSamples = static_cast<int>(sampleRate * 1.0);  
    delayBuffer.setSize(numChannels, maxDelaySamples);
    holdBuffer.setSize(numChannels, maxHoldSamples);
    loopBuffer.setSize(numChannels, (int)(sampleRate * 60.0)); 
    
    delayBuffer.clear(); holdBuffer.clear(); loopBuffer.clear(); 
    holdWriteIdx = 0; mdlPhase = 0.0f; mdlPhase2 = 0.0f; mdlPhase3 = 0.0f;
    recordedSamples = 0;
    grainClock = 0.0f;
    stepCount = 0;
    envFollower = 0.0f;
    transientDetected = false;
    holdLocked = false;

    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };

    // Máximo delay = modDepth máximo (50ms) + margen
    const int maxMdlDelaySamples = (int)(sampleRate * 0.1); // 100ms de margen
    mdlDelayL.prepare(spec);
    mdlDelayL.setMaximumDelayInSamples(maxMdlDelaySamples);
    mdlDelayR.prepare(spec);
    mdlDelayR.setMaximumDelayInSamples(maxMdlDelaySamples);
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

    grainFilters.assign(256, std::vector<FilterState>(numChannels));
    currentActiveGrainCount = 0.0f; interruptGate = 1.0f; interruptTimer = 0;
    for (auto& g : grains) g.active = false;

    grains.resize(256);

    activeGrainIndices.clear();
    freeGrainIndices.clear();
    activeGrainIndices.reserve((int)grains.size());
    freeGrainIndices.reserve((int)grains.size());

    for (int i = 0; i < (int)grains.size(); ++i)
    {
        grains[i].active = false;
        freeGrainIndices.push_back(i);
    }

    currentActiveGrainCount = 0.0f;
    interruptGate = 1.0f;
    interruptTimer = 0;

    pLooperQuant = apvts.getRawParameterValue("LOOPER_QUANT");
    pLooperRepeats = apvts.getRawParameterValue("REPEATS");
    pLooperSpray = apvts.getRawParameterValue("SPRAY");
    pLooperRev = apvts.getRawParameterValue("LOOPER_REV");
    pLooperSpread = apvts.getRawParameterValue("SPREAD");
    pLooperPitchJitter = apvts.getRawParameterValue("PITCH_JITTER");
    pLooperRevProb = apvts.getRawParameterValue("REV_PROB");
    pLooperWindowType = apvts.getRawParameterValue("WINDOW_TYPE");
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

void GranularEngine::scheduleGrains(float activity, float timeMs, float shape, int algo, int currentWriteIdx, const GrainParams& gp) {
    bool globalRev = gp.globalRev;
    bool globalQuant = gp.globalQuant;
    float repeats = gp.repeats;
    float spray = gp.spray;
    float spread = gp.spread;
    float jitter = gp.jitter;
    float revProb = gp.revProb;
    int globalWin = gp.globalWin;

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
        const int grainIndex = helpers.acquireFreeGrain(activeGrainIndices, freeGrainIndices, grains);
        if (grainIndex < 0)
            break; 

        auto& g = grains[(size_t)grainIndex];
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
            // Mosaic: Use octave-based speeds
            if (mode == 0) g.startSpeed = (dist(rng) < 0.5f) ? 1.0f : 2.0f;
            else if (mode == 1) g.startSpeed = (dist(rng) < 0.5f) ? 1.0f : 0.5f;
            else if (mode == 2) g.startSpeed = 2.0f;
            else { float ss[] = { 0.5f, 1.0f, 2.0f, 4.0f }; g.startSpeed = ss[(int)(dist(rng) * 4.0f)]; }
            g.endSpeed = g.startSpeed; g.length = lenBase * (0.5f + repeats * 3.0f);
        }
        else if (algo == 1) {
            // Seq: Random octaves or fifths
            g.length = lenBase * (0.25f + repeats * 2.0f);
            if (mode == 0) { g.filterCutoff = 0.2f + (1.0f - activity) * 0.5f; offset = (float)(dist(rng) * samplesPerStep * 8.0f); }
            else if (mode == 1) { 
                float choices[] = { 0.5f, 0.75f, 1.0f, 1.5f }; // Octave down, 5th down, Unity, 5th up
                g.startSpeed = choices[(int)(dist(rng) * 4.0f)]; 
                g.endSpeed = g.startSpeed; 
            }
        }
        else if (algo == 2) {
            // Glide: Exponential semitone glide
            g.length = lenBase * (0.5f + repeats * 10.0f);
            float startSemi = (mode == 0) ? -12.0f : (mode == 2 ? 7.0f : 0.0f);
            float endSemi = (mode == 1) ? 12.0f : (mode == 3 ? -24.0f : 0.0f);
            
            g.startSpeed = std::pow(2.0f, startSemi / 12.0f);
            // endSpeed stores the ratio for exponential glide: start * ratio^(age/length)
            g.endSpeed = std::pow(2.0f, (endSemi - startSemi) / 12.0f); 
            g.filterCutoff = 0.85f - (activity * 0.3f);
        }

        g.length = std::max(100.0f, std::min(g.length, (float)maxDelaySamples - 100.0f));

        // Exponential Jitter (semitones)
        if (jitter > 0.05f) {
            float jOff = (dist(rng) * 2.0f - 1.0f) * jitter * 12.0f; // Scale up to 1 octave
            g.startSpeed *= std::pow(2.0f, jOff / 12.0f);
        }

        // Global Pitch Drift (exponential)
        float driftVal = smoothDrift.getNextValue();
        if (driftVal > 0.01f) {
            float driftSemi = (dist(rng) * 2.0f - 1.0f) * driftVal * 2.0f;
            g.startSpeed *= std::pow(2.0f, driftSemi / 12.0f);
        }

        offset += 0.0075f * (float)fs;

        const float availableHistory = (float)std::max(1, recordedSamples - 4);
        offset = juce::jlimit(1.0f, availableHistory, offset);

        g.currentPos = (float)currentWriteIdx - offset;
        if (g.currentPos < 0.0f) g.currentPos += (float)maxDelaySamples;
        else if (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;
        g.startPos = g.currentPos;
        g.windowType = (globalWin > 0) ? globalWin - 1 : mode;
        for (auto& f : grainFilters[grainIndex]) { f.x1 = 0; f.y1 = 0; };
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
    
    if (wetBuffer.getNumChannels() != numChannels || wetBuffer.getNumSamples() < numSamples)
        wetBuffer.setSize(numChannels, numSamples, false, false, true);
    wetBuffer.clear();

    const GrainParams gp{
        pLooperRev->load() > 0.5f,
        pLooperQuant->load() > 0.5f,
        pLooperRepeats->load(),
        pLooperSpray->load(),
        pLooperSpread->load(),
        pLooperPitchJitter->load(),
        pLooperRevProb->load(),
        (int)pLooperWindowType->load()
    };

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
        float currentMdlDelay = std::abs((depthSamples + 10.0f) + (compoundLfo * depthSamples));
 
        scheduleGrains(activityVal, effectiveTimeMs, smoothShape.getNextValue(), algo, writeIdx, gp);
        
        if (algo == 7) { 
            if (interruptTimer > 0) { interruptGate = juce::jmax(0.0f, interruptGate - 0.01f); interruptTimer--; } 
            else { interruptGate = juce::jmin(1.0f, interruptGate + 0.005f); } 
        } else { interruptGate = 1.0f; interruptTimer = 0; }

        int activeGrainsThisSample = 0;
        float winSumL = 0.0f;
        float winSumR = 0.0f;
        float currentSampleL = 0.0f;
        float currentSampleR = 0.0f;

        for (int activePos = 0; activePos < (int)activeGrainIndices.size();)
        {
            const int grainIndex = activeGrainIndices[(size_t)activePos];
            auto& g = grains[(size_t)grainIndex];
            float phase = g.age / g.length;
            float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
            
            float gainL = window * (1.0f - g.pan);
            float gainR = window * g.pan;
            
            winSumL += gainL;
            winSumR += gainR;

            float sL = bufferReaders.getNextSampleLinear(0, g.currentPos, delayBuffer, maxDelaySamples) * gainL;
            float sR = bufferReaders.getNextSampleLinear(numChannels > 1 ? 1 : 0, g.currentPos, delayBuffer, maxDelaySamples) * gainR;

            currentSampleL += sL;
            if (numChannels > 1) currentSampleR += sR;

            ++activeGrainsThisSample;
            if (algo == 2) {
                // Exponential glide: speed = startSpeed * (ratio ^ progress)
                float progress = g.age / g.length;
                float instSpeed = g.startSpeed * std::pow(g.endSpeed, progress);
                g.currentPos += g.reverse ? -instSpeed : instSpeed;
            }
            else {
                g.currentPos += g.reverse ? -g.startSpeed : g.startSpeed;
            }

            if (g.currentPos < 0.0f) g.currentPos += (float)maxDelaySamples;
            else if (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;
            if (++g.age >= g.length) {
                helpers.releaseActiveGrainByActiveListPosition(activeGrainIndices, freeGrainIndices, grains, activePos);
                continue;
            }
            ++activePos;
        }
        
        // Smoothed Peak Tracking for the Density Meter
        float instantCount = (float)activeGrainsThisSample;
        float alpha = instantCount > currentActiveGrainCount ? 0.1f : 0.005f; 
        currentActiveGrainCount = alpha * instantCount + (1.0f - alpha) * currentActiveGrainCount;

        float mix = smoothMix.getNextValue(), gain = smoothGain.getNextValue();
        float masterWet = smoothMasterWetVol.getNextValue();
        float repeats = smoothRepeats.getNextValue();
        int activeChannels = std::min(numChannels, preparedChannels);

        // Linear Normalization for Unity Gain (mathematically matches input level)
        float normL = winSumL > 0.001f ? 1.0f / winSumL : 1.0f;
        float normR = winSumR > 0.001f ? 1.0f / winSumR : 1.0f;

        for (int ch = 0; ch < activeChannels; ++ch) {
            float dryInput = buffer.getSample(ch, i);
            
            // Apply normalization and a safety saturation stage
            float normalizedWet = (ch == 0 ? currentSampleL * normL : currentSampleR * normR);
            float wetRaw = std::tanh(normalizedWet); 
            
            auto& mdlDelay = (ch == 0) ? mdlDelayL : mdlDelayR;
            mdlDelay.setDelay(currentMdlDelay);
            mdlDelay.pushSample(ch, wetRaw);
            float wet = mdlDelay.popSample(ch);
            if (!std::isfinite(wet)) wet = 0.0f;
            
            // Final output mix
            buffer.setSample(ch, i, (dryInput * (1.0f - mix) + (wet * masterWet) * mix) * gain);
            
            if (!freeze) {
                // Feedback remains stable and within [-1, 1] range
                float fb = wet * repeats * 0.98f; 
                if (!std::isfinite(fb)) fb = 0.0f;
                delayBuffer.setSample(ch, writeIdx, std::tanh(dryInput + fb));
            }
        }
        //mdlWriteIdx = (mdlWriteIdx + 1) % 16384;
        if (!freeze)
        {
            writeIdx = (writeIdx + 1) % maxDelaySamples;
            recordedSamples = std::min(recordedSamples + 1, maxDelaySamples);
        }
    }
    
    juce::dsp::AudioBlock<float> outputBlock(buffer); 
    
    // Update Filter and FX parameters before processing
    float filterVal = smoothFilter.getNextValue();
    float resVal = apvts.getRawParameterValue("RESONANCE")->load();
    float spaceVal = smoothSpace.getNextValue();
    
    // Logarithmic mapping for musical filter cutoff (20Hz - 20kHz)
    float cutoffFreq = 20.0f * std::pow(1000.0f, filterVal);
    auto& filter = fxChain.get<0>();
    filter.setCutoffFrequencyHz(cutoffFreq);
    filter.setResonance(resVal * 0.9f); // Keep resonance safe
    
    // Update Reverb (Space)
    auto& reverb = fxChain.get<1>();
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = spaceVal * 0.95f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = spaceVal * 0.5f;
    reverbParams.dryLevel = 1.0f - (spaceVal * 0.3f);
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
    
    // Process the FX chain
    fxChain.process(juce::dsp::ProcessContextReplacing<float>(outputBlock));
    
    limiter.process(juce::dsp::ProcessContextReplacing<float>(outputBlock));
}
