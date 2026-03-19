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
    wetBuffer.setSize(preparedChannels, samplesPerBlock);
    wetBuffer.clear();
    
    maxDelaySamples = static_cast<int>(sampleRate * 2.0); 
    maxHoldSamples = static_cast<int>(sampleRate * 1.0);  
    delayBuffer.setSize(numChannels, maxDelaySamples);
    holdBuffer.setSize(numChannels, maxHoldSamples);
    loopBuffer.setSize(numChannels, (int)(sampleRate * 30.0)); 
    
    delayBuffer.clear(); holdBuffer.clear(); loopBuffer.clear(); 
    holdWriteIdx = 0; mdlPhase = 0.0f; mdlPhase2 = 0.0f; mdlPhase3 = 0.0f;
    recordedSamples = 0;
    grainClock = 0.0f;
    stepCount = 0;
    envFollower = 0.0f;
    transientDetected = false;
    holdLocked = false;

    // Cache Parameter Pointers (Audio Thread Safe)
    pActivity = apvts.getRawParameterValue("ACTIVITY");
    pTime = apvts.getRawParameterValue("TIME");
    pShape = apvts.getRawParameterValue("SHAPE");
    pRepeats = apvts.getRawParameterValue("REPEATS");
    pFilter = apvts.getRawParameterValue("FILTER");
    pSpace = apvts.getRawParameterValue("SPACE");
    pMix = apvts.getRawParameterValue("MIX");
    pGain = apvts.getRawParameterValue("GAIN");
    pMasterWet = apvts.getRawParameterValue("MASTER_WET_VOL");
    pLoopLevel = apvts.getRawParameterValue("LOOP_LEVEL");
    pSpray = apvts.getRawParameterValue("SPRAY");
    pSpread = apvts.getRawParameterValue("SPREAD");
    pPitchJitter = apvts.getRawParameterValue("PITCH_JITTER");
    pRevProb = apvts.getRawParameterValue("REV_PROB");
    pModRate = apvts.getRawParameterValue("MOD_RATE");
    pModDepth = apvts.getRawParameterValue("MOD_DEPTH");
    pAlgo = apvts.getRawParameterValue("ALGO");
    pFreeze = apvts.getRawParameterValue("FREEZE");
    pResonance = apvts.getRawParameterValue("RESONANCE");
    
    pLooperQuant = apvts.getRawParameterValue("LOOPER_QUANT");
    pLooperRev = apvts.getRawParameterValue("LOOPER_REV");
    pLooperWindowType = apvts.getRawParameterValue("WINDOW_TYPE");

    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };

    // Máximo delay = modDepth máximo (50ms) + margen / Maximum delay = maximum modDepth (50ms) + margin
    const int maxMdlDelaySamples = (int)(sampleRate * 0.1); // 100ms de margen / 100ms margin
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

    grainFilters.assign(MAX_GRAINS, std::vector<FilterState>(numChannels));
    currentActiveGrainCount = 0.0f; interruptGate = 1.0f; interruptTimer = 0;
    
    grains.resize(MAX_GRAINS);
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
        const int grainIndex = helpers.acquireFreeGrain();
        if (grainIndex < 0)
            break; 

        auto& g = grains[(size_t)grainIndex];
        g.active = true; g.age = 0.0f; 
        g.velocity = 1.0f; g.targetVelocity = 1.0f; g.acceleration = 0.0f;
        g.internalPhase = dist(rng);
        g.reverse = (dist(rng) < revProb) ? !globalRev : globalRev;
        g.filterCutoff = 1.0f; g.bitCrush = 1.0f; g.isSustainer = false;

        float lenBase = timeMs * ((float)fs / 1000.0f) * (1.0f + activity * 1.5f);
        float random_spread = (dist(rng) * 2.0f - 1.0f) * activity * 1000.0f;
        float offset = dist(rng) * (float)fs * (0.1f + activity * 1.0f) + random_spread;

        offset += (dist(rng) * 2.0f - 1.0f) * spray * (float)fs * 0.5f;
        g.pan = juce::jlimit(0.0f, 1.0f, 0.5f + (dist(rng) - 0.5f) * (spread + activity * 0.2f));

        if (algo == 0) {
            // Mosaic: Overlapping loops at different speeds
            if (mode == 0)      g.velocity = (dist(rng) < 0.5f) ? 1.0f : 2.0f; 
            else if (mode == 1) g.velocity = (dist(rng) < 0.5f) ? 1.0f : 0.5f; 
            else if (mode == 2) g.velocity = 2.0f;                             
            else { 
                float ss[] = { 0.5f, 1.0f, 2.0f }; 
                g.velocity = ss[(int)(dist(rng) * 3.0f)];                     
            }
            g.targetVelocity = g.velocity; 
            g.length = lenBase * (0.8f + repeats * 4.0f);
        }
        else if (algo == 1) {
            // Seq: Rearranged rhythmic sequences
            g.length = lenBase * (0.4f + repeats * 2.0f);
            if (mode == 0) {
                g.velocity = (stepCount % 2 == 0) ? 1.0f : 0.5f;
                if (dist(rng) > activity) g.velocity = 1.0f; 
            }
            else if (mode == 1) {
                float arp[] = { 1.0f, 1.25f, 1.5f, 2.0f }; 
                g.velocity = arp[stepCount % 4];
            }
            else if (mode == 2) {
                g.velocity = (stepCount % 2 == 0) ? 1.5f : 1.0f; 
            }
            else {
                float choices[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
                g.velocity = choices[(int)(dist(rng) * 6.0f)];
            }
            g.targetVelocity = g.velocity;
        }
        else if (algo == 2) {
            // Glide: Ballistic Pitch Trajectory (Expert Physics)
            g.length = lenBase * (1.0f + repeats * 8.0f);
            g.velocity = 1.0f;
            if (mode == 0)      g.targetVelocity = 0.5f; 
            else if (mode == 1) g.targetVelocity = 2.0f; 
            else if (mode == 2) g.targetVelocity = (dist(rng) < 0.5f) ? 1.2f : 0.8f; 
            else {
                float intervals[] = { 1.0f, 1.25f, 1.5f, 2.0f, 1.0f, 0.75f, 0.5f };
                g.velocity = intervals[stepCount % 7];
                g.targetVelocity = g.velocity;
            }
            // Acceleration (dV/dt) over grain duration
            g.acceleration = (g.targetVelocity - g.velocity) / g.length;
        }
        else if (algo == 3) {
            // Haze: Atmospheric clouds
            g.length = lenBase * (1.5f + repeats * 5.0f);
            if (mode == 0)      g.velocity = 1.0f; 
            else if (mode == 1) g.velocity = (dist(rng) < 0.6f) ? 0.5f : 1.0f; 
            else if (mode == 2) g.velocity = (dist(rng) < 0.6f) ? 2.0f : 1.0f; 
            else {
                float ss[] = { 0.5f, 1.0f, 2.0f };
                g.velocity = ss[(int)(dist(rng) * 3.0f)]; 
            }
            g.targetVelocity = g.velocity;
            g.filterCutoff = 0.5f + (1.0f - activity) * 0.4f;
        }
        else if (algo == 4) {
            // Tunnel: Resonant drones
            g.length = lenBase * (4.0f + repeats * 10.0f);
            if (mode == 0)      g.velocity = 1.0f;
            else if (mode == 1) g.velocity = 0.5f; 
            else if (mode == 2) {
                g.length *= (0.8f + 0.4f * std::sin(grainClock * 0.1f));
                g.velocity = 1.0f;
            }
            else {
                g.velocity = 1.0f + (std::sin(grainClock * 0.05f) * 0.05f);
            }
            g.targetVelocity = g.velocity;
        }
        else if (algo == 5) {
            // Strum: Pointillistic plucked textures
            g.length = 1500.0f + (activity * 3000.0f); 
            if (mode == 0)      g.velocity = 1.0f;
            else if (mode == 1) g.velocity = 0.5f; 
            else if (mode == 2) g.velocity = 2.0f; 
            else {
                float ss[] = { 0.5f, 1.0f, 2.0f, 4.0f };
                g.velocity = ss[(int)(dist(rng) * 4.0f)]; 
            }
            g.targetVelocity = g.velocity;
            g.windowType = 2; 
        }
        else if (algo == 6) {
            // Blocks: Predictable glitches
            g.length = samplesPerStep * (0.5f + activity); 
            if (mode == 0)      g.velocity = 1.0f;
            else if (mode == 1) g.velocity = 0.5f; 
            else if (mode == 2) g.velocity = 2.0f; 
            else {
                g.velocity = (dist(rng) < 0.5f) ? 1.5f : 1.0f;
                g.reverse = (dist(rng) < 0.3f); 
            }
            g.targetVelocity = g.velocity;
            g.windowType = 3; 
        }
        else if (algo == 7) {
            // Interrupt: Sparse signal replacements
            g.length = lenBase * 0.5f;
            if (mode == 0)      { g.velocity = 1.0f; }
            else if (mode == 1) { g.velocity = 1.0f; g.filterCutoff = 0.2f; } 
            else if (mode == 2) { g.velocity = 1.0f; g.filterCutoff = 0.8f; g.brightness = 2.0f; } 
            else {
                g.velocity = (dist(rng) < 0.5f) ? 0.5f : 2.0f; 
                g.length *= 0.5f;
            }
            g.targetVelocity = g.velocity;
        }
        else if (algo == 8) {
            // Arp: Sequenced runs
            g.length = lenBase * 0.3f;
            if (mode == 0) {
                float seq[] = { 1.0f, 1.25f, 1.5f, 2.0f, 1.5f, 1.25f };
                g.velocity = seq[stepCount % 6];
            }
            else if (mode == 1) {
                float seq[] = { 1.0f, 1.25f, 1.5f, 1.875f }; 
                g.velocity = seq[stepCount % 4];
            }
            else if (mode == 2) {
                float seq[] = { 1.0f, 1.2f, 1.5f, 1.8f }; 
                g.velocity = seq[stepCount % 4];
            }
            else {
                static int lastStep = 0;
                lastStep = juce::jlimit(0, 7, lastStep + (dist(rng) < 0.5f ? -1 : 1));
                float scales[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 4.0f };
                g.velocity = scales[lastStep];
            }
            g.targetVelocity = g.velocity;
        }
        else if (algo == 9) {
            // Pattern: Rhythmic delay taps
            g.length = lenBase * 0.5f;
            if (mode == 0)      offset = samplesPerStep * 3.0f; 
            else if (mode == 1) offset = samplesPerStep * 0.666f; 
            else if (mode == 2) offset = samplesPerStep * (float)(stepCount % 4 + 1) * 0.5f; 
            else                offset = (float)fs * 0.05f * (float)(dist(rng) * 10.0f); 
            
            g.velocity = 1.0f; g.targetVelocity = 1.0f;
        }
        else if (algo == 10) {
            // Warp: Tape Drift
            g.length = lenBase * 0.8f;
            g.velocity = 1.0f;
            if (mode == 1) g.targetVelocity = 0.95f; 
            else if (mode == 2) g.targetVelocity = 1.05f; 
            else {
                float flutter = std::sin(grainClock * 0.2f) * 0.02f;
                g.velocity = 1.0f + flutter;
                g.targetVelocity = g.velocity;
            }
            g.acceleration = (g.targetVelocity - g.velocity) / g.length;
        }

        g.length = std::max(100.0f, std::min(g.length, (float)maxDelaySamples - 100.0f));

        // Exponential Jitter (semitones)
        if (jitter > 0.05f) {
            float jOff = (dist(rng) * 2.0f - 1.0f) * jitter * 12.0f; // Scale up to 1 octave
            if (globalQuant) jOff = std::round(jOff); // Snap to semitones if Quantize is on
            g.startSpeed *= std::pow(2.0f, jOff / 12.0f);
        }

        // Global Pitch Drift (exponential)
        float driftVal = smoothDrift.getNextValue();
        if (driftVal > 0.01f) {
            float driftSemi = (dist(rng) * 2.0f - 1.0f) * driftVal * 2.0f;
            if (globalQuant) driftSemi = std::round(driftSemi); // Snap to semitones if Quantize is on
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
    const int numSamples = buffer.getNumSamples(); 
    const int numChannels = buffer.getNumChannels();
    
    wetBuffer.clear();

    updateEnvelopeFollower(buffer);
    
    // Cache Parameter Values (Atomic loads once per block - FAST)
    const float activityVal = pActivity->load();
    const float timeVal = pTime->load();
    const float shapeVal = pShape->load();
    const float repeatsVal = pRepeats->load();
    const float filterVal = pFilter->load();
    const float spaceVal = pSpace->load();
    const float mixVal = pMix->load();
    const float gainVal = pGain->load();
    const float masterWet = pMasterWet->load();
    const float loopLevelVal = pLoopLevel->load();
    const float sprayVal = pSpray->load();
    const float spreadVal = pSpread->load();
    const float jitterVal = pPitchJitter->load();
    const float revProbVal = pRevProb->load();
    const float modRateVal_raw = pModRate->load();
    const float modDepthVal_raw = pModDepth->load();
    const int algo = (int)pAlgo->load();
    const bool freeze = pFreeze->load() > 0.5f;

    smoothActivity.setTargetValue(activityVal);
    smoothTime.setTargetValue(timeVal);
    smoothShape.setTargetValue(shapeVal);
    smoothRepeats.setTargetValue(repeatsVal);
    smoothFilter.setTargetValue(filterVal);
    smoothSpace.setTargetValue(spaceVal);
    smoothMix.setTargetValue(mixVal);
    smoothGain.setTargetValue(gainVal);
    smoothMasterWetVol.setTargetValue(masterWet);
    smoothLoopLevel.setTargetValue(loopLevelVal);
    smoothSpray.setTargetValue(sprayVal);
    smoothSpread.setTargetValue(spreadVal);
    smoothPitchJitter.setTargetValue(jitterVal);
    smoothRevProb.setTargetValue(revProbVal);
    smoothModRate.setTargetValue(modRateVal_raw);
    smoothModDepth.setTargetValue(modDepthVal_raw);

    const GrainParams gp{
        pLooperRev->load() > 0.5f,
        pLooperQuant->load() > 0.5f,
        repeatsVal,
        sprayVal,
        spreadVal,
        jitterVal,
        revProbVal,
        (int)pLooperWindowType->load()
    };

    const float effectiveTimeMs = 50.0f + (timeVal * 1950.0f);
    int activeGrainsThisBlock = 0;

    for (int i = 0; i < numSamples; ++i) {
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
 
        scheduleGrains(activityVal, effectiveTimeMs, shapeVal, algo, writeIdx, gp);
        
        if (algo == 7) { 
            if (interruptTimer > 0) { interruptGate = juce::jmax(0.0f, interruptGate - 0.01f); interruptTimer--; } 
            else { interruptGate = juce::jmin(1.0f, interruptGate + 0.005f); } 
        } else { interruptGate = 1.0f; interruptTimer = 0; }

        int activeGrainsThisSample = 0;
        float winSumL = 0.0f, winSumR = 0.0f;
        float currentSampleL = 0.0f, currentSampleR = 0.0f;

        for (int grainIndex = 0; grainIndex < MAX_GRAINS; ++grainIndex)
        {
            auto& g = grains[(size_t)grainIndex];
            if (!g.active) continue;

            float phase = g.age / g.length;
            float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
            
            float gainL = window * (1.0f - g.pan), gainR = window * g.pan;
            winSumL += gainL; winSumR += gainR;

            float sL = bufferReaders.getNextSampleLinear(0, g.currentPos, delayBuffer, maxDelaySamples) * gainL;
            float sR = bufferReaders.getNextSampleLinear(numChannels > 1 ? 1 : 0, g.currentPos, delayBuffer, maxDelaySamples) * gainR;

            auto& states = grainFilters[(size_t)grainIndex];
            float lpAlpha = g.filterCutoff;
            states[0].y1 = states[0].y1 + lpAlpha * (sL - states[0].y1);
            sL = states[0].y1 * g.brightness;
            if (numChannels > 1) {
                states[1].y1 = states[1].y1 + lpAlpha * (sR - states[1].y1);
                sR = states[1].y1 * g.brightness;
            }

            currentSampleL += sL;
            if (numChannels > 1) currentSampleR += sR;

            activeGrainsThisSample++;
            
            // --- KINEMATIC INTEGRATOR (Expert Phase Accuracy) ---
            g.velocity += g.acceleration; 
            g.currentSpeed = g.velocity;
            g.currentPos += g.reverse ? -g.velocity : g.velocity;

            if (g.currentPos < 0.0f) g.currentPos += (float)maxDelaySamples;
            else if (g.currentPos >= (float)maxDelaySamples) g.currentPos -= (float)maxDelaySamples;
            
            g.age += 1.0f;
            if (g.age >= g.length) {
                g.active = false;
                helpers.releaseGrain(grainIndex);
            }
        }
        
        activeGrainsThisBlock = std::max(activeGrainsThisBlock, activeGrainsThisSample);

        float densityFactor = std::sqrt(juce::jmax(1.0f, currentActiveGrainCount));
        float normL = winSumL > 0.001f ? (1.0f / (winSumL * 0.5f + densityFactor * 0.5f)) : 1.0f;
        float normR = winSumR > 0.001f ? (1.0f / (winSumR * 0.5f + densityFactor * 0.5f)) : 1.0f;

        float densityProtection = 1.0f / (1.0f + (currentActiveGrainCount * 0.001f));
        normL *= densityProtection; normR *= densityProtection;

        int activeChannels = std::min(numChannels, preparedChannels);
        for (int ch = 0; ch < activeChannels; ++ch) {
            float dryInput = buffer.getSample(ch, i);
            float normalizedWet = (ch == 0 ? currentSampleL * normL : currentSampleR * normR);
            float wetRaw = std::tanh(normalizedWet); 
            
            auto& mdlDelay = (ch == 0) ? mdlDelayL : mdlDelayR;
            mdlDelay.setDelay(currentMdlDelay);
            mdlDelay.pushSample(ch, wetRaw);
            float wet = mdlDelay.popSample(ch);
            if (!std::isfinite(wet)) wet = 0.0f;
            
            buffer.setSample(ch, i, (dryInput * (1.0f - mixVal) + (wet * masterWet) * mixVal) * gainVal);
            
            if (!freeze) {
                float fb = wet * repeatsVal * 0.98f; 
                if (!std::isfinite(fb)) fb = 0.0f;
                delayBuffer.setSample(ch, writeIdx, std::tanh(dryInput + fb));
            }
        }
        
        if (!freeze) {
            writeIdx = (writeIdx + 1) % maxDelaySamples;
            recordedSamples = std::min(recordedSamples + 1, maxDelaySamples);
        }
    }

    float alphaCount = activeGrainsThisBlock > currentActiveGrainCount ? 0.1f : 0.005f; 
    currentActiveGrainCount = alphaCount * (float)activeGrainsThisBlock + (1.0f - alphaCount) * currentActiveGrainCount;
    
    juce::dsp::AudioBlock<float> outputBlock(buffer); 
    const float resVal = pResonance->load();
    
    float cutoffFreq = 20.0f * std::pow(1000.0f, filterVal);
    auto& filter = fxChain.get<0>();
    filter.setCutoffFrequencyHz(cutoffFreq);
    filter.setResonance(resVal * 0.9f);
    
    auto& reverb = fxChain.get<1>();
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = spaceVal * 0.95f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = spaceVal * 0.5f;
    reverbParams.dryLevel = 1.0f - (spaceVal * 0.3f);
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
    
    fxChain.process(juce::dsp::ProcessContextReplacing<float>(outputBlock));
    limiter.process(juce::dsp::ProcessContextReplacing<float>(outputBlock));

    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = std::max(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    float alphaOut = peak > outputFollower ? 0.05f : 0.005f; 
    outputFollower = alphaOut * peak + (1.0f - alphaOut) * outputFollower;
}
