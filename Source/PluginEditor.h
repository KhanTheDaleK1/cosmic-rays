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

    juce::Slider activitySlider, shapeSlider, filterSlider, mixSlider;
    juce::ComboBox algoBox;
    juce::Label activityLabel, shapeLabel, filterLabel, mixLabel, algoLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> activityAttach, shapeAttach, filterAttach, mixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CosmicRaysAudioProcessorEditor)
};
