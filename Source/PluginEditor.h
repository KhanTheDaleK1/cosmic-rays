#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class CosmicRaysAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor&);
    ~CosmicRaysAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CosmicRaysAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CosmicRaysAudioProcessorEditor)
};
