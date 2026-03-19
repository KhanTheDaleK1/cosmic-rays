#pragma once

#include "Version.h"
#include <juce_gui_basics/juce_gui_basics.h>

#define ENABLE_ENHANCED_VISUALIZERS 1

class CosmicRaysAudioProcessor;

class CustomLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CustomLookAndFeel();
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    struct Colors {
        static inline const juce::Colour agedPlastic  { 0xFFC5B381 };
        static inline const juce::Colour puttyGrey     { 0xFFBDB7AB };
        static inline const juce::Colour appleGreen    { 0xFF61BB46 };
        static inline const juce::Colour appleYellow   { 0xFFFDB827 };
        static inline const juce::Colour appleOrange   { 0xFFF58220 };
        static inline const juce::Colour appleRed      { 0xFFE03A3E };
        static inline const juce::Colour applePurple   { 0xFF963D97 };
        static inline const juce::Colour appleBlue     { 0xFF009DDC };
        static inline const juce::Colour phosphorGreen { 0xFF33FF33 };
        static inline const juce::Colour scopeBg       { 0xFF000F00 };
    };
};

class FilterVisualizer : public juce::Component {
public:
    FilterVisualizer(CosmicRaysAudioProcessor& p) : audioProcessor(p) {}
    void paint(juce::Graphics& g) override;
private:
    CosmicRaysAudioProcessor& audioProcessor;
};

class PitchVisualizer : public juce::Component {
public:
    PitchVisualizer(CosmicRaysAudioProcessor& p) : audioProcessor(p) {}
    void paint(juce::Graphics& g) override;
private:
    CosmicRaysAudioProcessor& audioProcessor;
};

class DensityMeter : public juce::Component {
public:
    DensityMeter(CosmicRaysAudioProcessor& p) : audioProcessor(p) {}
    void paint(juce::Graphics& g) override;
private:
    CosmicRaysAudioProcessor& audioProcessor;
};

class WaveformVisualizer : public juce::Component {
public:
    WaveformVisualizer(CosmicRaysAudioProcessor& p) : audioProcessor(p) {}
    void paint(juce::Graphics& g) override;
private:
    CosmicRaysAudioProcessor& audioProcessor;
};

class HelpComponent : public juce::Component {
public:
    HelpComponent(const juce::String& version);
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    juce::TextEditor textEditor;
};


// ... other includes if they were there ...

class CosmicRaysAudioProcessorEditor  : public juce::AudioProcessorEditor,
 public juce::Timer
{
public:
    CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor&);
    ~CosmicRaysAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    bool keyPressed (const juce::KeyPress& key) override;
    void modifierKeysChanged (const juce::ModifierKeys& modifiers) override;

private:
    juce::String currentVersion = PROJECT_VERSION_STRING;
    CustomLookAndFeel customLookAndFeel;
    CosmicRaysAudioProcessor& audioProcessor;

    FilterVisualizer filterVis;
    PitchVisualizer pitchVis;
    DensityMeter densityMeter;
    WaveformVisualizer waveformVis;
    HelpComponent helpOverlay;

    juce::Slider activitySlider, repeatsSlider, filterSlider, spaceSlider, mixSlider, loopLevelSlider, gainSlider;
    juce::Slider modRateSlider, modDepthSlider, spraySlider, spreadSlider;
    juce::Slider jitterSlider, revProbSlider;
    juce::ComboBox algoBox, looperModeBox, windowTypeBox;
    juce::GroupComponent looperBox, timeBox, shapeBox;
    juce::TextButton helpButton, shiftButton, tapButton, looperRecButton, looperOdubButton, freezeButton; 
    juce::TextButton undoButton, redoButton, feedbackButton, updateButton;
    juce::TextButton quantizeButton, reverseButton;
    juce::TextButton time1_4Button, time1_2Button, time1xButton, time2xButton, time4xButton, time8xButton;
    juce::TextButton shapeAButton, shapeBButton, shapeCButton, shapeDButton;
    juce::ToggleButton killDryButton, trailsButton;

    juce::Label activityLabel, repeatsLabel, filterLabel, spaceLabel, mixLabel, loopLevelLabel, algoLabel, gainLabel, looperModeLabel;
    juce::Label modRateLabel, modDepthLabel, killDryLabel, trailsLabel, sprayLabel, spreadLabel;
    juce::Label jitterLabel, revProbLabel;
    juce::Label resLabel, modRateLabelHeader, modDepthLabelHeader;
    juce::Label cpuLabel, ramLabel;
    juce::Label waveformVisLabel, pitchVisLabel, filterVisLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> activityAttach, repeatsAttach, filterAttach, spaceAttach, mixAttach, loopLevelAttach, gainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modRateAttach, modDepthAttach, sprayAttach, spreadAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> jitterAttach, revProbAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> algoAttach, looperModeAttach, windowTypeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> quantizeAttach, reverseAttach, freezeAttach, looperRecAttach, looperOdubAttach, killDryAttach, trailsAttach;

    bool isShiftPressed = false;
    bool isFineMode = false;
    void updateLabels();

    juce::Image plasticTexture;
    juce::Image logoImage;
    void createPlasticTexture();

    // Tap Tempo State
    juce::Array<double> tapTimes;
    void handleTap();

    bool ledOn = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CosmicRaysAudioProcessorEditor)
};