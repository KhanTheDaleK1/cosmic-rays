/*
  ==============================================================================

    getNextSampleLinear.h
    Created: 10 Mar 2026 11:13:24am
    Author:  diego

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class BufferReaders {
public:
    float getNextSampleLinear(int channel, float readPos, juce::AudioBuffer<float>& delayBuffer, int& maxDelaySamples) noexcept
    {
        if (channel >= delayBuffer.getNumChannels())
            channel = 0;

        const int i0 = (int)readPos;
        const float frac = readPos - (float)i0;

        int p0 = i0;
        int p1 = i0 + 1;

        if (p0 >= maxDelaySamples) p0 -= maxDelaySamples;
        if (p0 < 0) p0 += maxDelaySamples;
        if (p1 >= maxDelaySamples) p1 -= maxDelaySamples;
        if (p1 < 0) p1 += maxDelaySamples;

        const float s0 = delayBuffer.getSample(channel, p0);
        const float s1 = delayBuffer.getSample(channel, p1);
        return s0 + frac * (s1 - s0);
    }
};