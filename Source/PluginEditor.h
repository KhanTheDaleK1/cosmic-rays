#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
};

class CosmicRaysAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor&);
    ~CosmicRaysAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    CustomLookAndFeel customLookAndFeel;
    CosmicRaysAudioProcessor& processor;

    juce::Slider activitySlider, timeSlider, shapeSlider, repeatsSlider, filterSlider, spaceSlider, mixSlider, loopLevelSlider, gainSlider;
    juce::ComboBox algoBox, looperModeBox, presetBox;
    juce::TextButton shiftButton, tapButton; 
    juce::ToggleButton quantizeButton, reverseButton;

    juce::Label activityLabel, timeLabel, shapeLabel, repeatsLabel, filterLabel, spaceLabel, mixLabel, loopLevelLabel, algoLabel, gainLabel, looperModeLabel;
    juce::Label resLabel, modRateLabel, modDepthLabel, spaceModeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> activityAttach, timeAttach, shapeAttach, repeatsAttach, filterAttach, spaceAttach, mixAttach, loopLevelAttach, gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoAttach, looperModeAttach, presetAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> quantizeAttach, reverseAttach;

    bool isShiftPressed = false;
    void updateLabels();

    // Tap Tempo State
    juce::Array<double> tapTimes;
    void handleTap();

    bool ledOn = false;
    uint32_t lastBlinkTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CosmicRaysAudioProcessorEditor)
};