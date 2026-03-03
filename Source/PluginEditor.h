#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CustomLookAndFeel();
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    
    struct Colors {
        static inline const juce::Colour appleBeige   { 0xFFCECECE };
        static inline const juce::Colour appleGreen   { 0xFF61BB46 };
        static inline const juce::Colour appleYellow  { 0xFFFDB827 };
        static inline const juce::Colour appleOrange  { 0xFFF58220 };
        static inline const juce::Colour appleRed     { 0xFFE03A3E };
        static inline const juce::Colour applePurple  { 0xFF963D97 };
        static inline const juce::Colour appleBlue    { 0xFF009DDC };
        static inline const juce::Colour phosphorGreen { 0xFF33FF33 };
        static inline const juce::Colour scopeBg      { 0xFF001100 };
    };
};

class FilterVisualizer : public juce::Component {
public:
    FilterVisualizer(CosmicRaysAudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics& g) override;
private:
    CosmicRaysAudioProcessor& processor;
};

class GrainShapeVisualizer : public juce::Component {
public:
    GrainShapeVisualizer(CosmicRaysAudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics& g) override;
private:
    CosmicRaysAudioProcessor& processor;
};

class HelpComponent : public juce::Component {
public:
    HelpComponent();
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::TextEditor textEditor;
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

    FilterVisualizer filterVis;
    GrainShapeVisualizer grainShapeVis;
    HelpComponent helpOverlay;

    juce::Slider activitySlider, timeSlider, shapeSlider, repeatsSlider, filterSlider, spaceSlider, mixSlider, loopLevelSlider, gainSlider;
    juce::Slider modRateSlider, modDepthSlider;
    juce::ComboBox algoBox, looperModeBox, presetBox;
    juce::TextButton shiftButton, tapButton, freezeButton, helpButton; 
    juce::TextButton looperRecButton, looperOdubButton;
    juce::ToggleButton quantizeButton, reverseButton, killDryButton, trailsButton;

    juce::Label activityLabel, timeLabel, shapeLabel, repeatsLabel, filterLabel, spaceLabel, mixLabel, loopLevelLabel, algoLabel, gainLabel, looperModeLabel;
    juce::Label modRateLabel, modDepthLabel, killDryLabel, trailsLabel;
    juce::Label resLabel, modRateLabelHeader, modDepthLabelHeader;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> activityAttach, timeAttach, shapeAttach, repeatsAttach, filterAttach, spaceAttach, mixAttach, loopLevelAttach, gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modRateAttach, modDepthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoAttach, looperModeAttach, presetAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> quantizeAttach, reverseAttach, freezeAttach, looperRecAttach, looperOdubAttach, killDryAttach, trailsAttach;

    bool isShiftPressed = false;
    void updateLabels();

    // Tap Tempo State
    juce::Array<double> tapTimes;
    void handleTap();

    bool ledOn = false;
    uint32_t lastBlinkTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CosmicRaysAudioProcessorEditor)
};