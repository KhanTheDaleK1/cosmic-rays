#include "PluginProcessor.h"
#include "PluginEditor.h"

CosmicRaysAudioProcessorEditor::CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // --- Style Settings ---
    auto knobColor = juce::Colour::grey (0.8f);
    auto accentColor = juce::Colours::teal;

    // Helper to setup knobs
    auto setupKnob = [&](juce::Slider& s, juce::Label& l, const juce::String& name) {
        addAndMakeVisible (s);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setColour (juce::Slider::rotarySliderFillColourId, accentColor);
        s.setColour (juce::Slider::thumbColourId, knobColor);

        addAndMakeVisible (l);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (14.0f, juce::Font::bold));
        l.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
    };

    setupKnob (activitySlider, activityLabel, "Activity");
    setupKnob (shapeSlider, shapeLabel, "Shape");
    setupKnob (filterSlider, filterLabel, "Filter");
    setupKnob (mixSlider, mixLabel, "Mix");

    addAndMakeVisible (algoBox);
    algoBox.addItemList ({"Mosaic", "Glitch", "Warp", "Ghost"}, 1);
    algoBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha (0.4f));

    addAndMakeVisible (algoLabel);
    algoLabel.setText ("Algorithm", juce::dontSendNotification);
    algoLabel.setJustificationType (juce::Justification::centred);
    algoLabel.setColour (juce::Label::textColourId, juce::Colours::lightgrey);

    // --- Attachments ---
    activityAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ACTIVITY", activitySlider);
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SHAPE", shapeSlider);
    filterAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "FILTER", filterSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "MIX", mixSlider);
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "ALGO", algoBox);

    setSize (600, 400);
}

CosmicRaysAudioProcessorEditor::~CosmicRaysAudioProcessorEditor() {}

void CosmicRaysAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background: Dark Sleek Gradient (GarageBand style)
    g.fillAll (juce::Colour (0xFF1A1A1A));
    
    auto bounds = getLocalBounds().toFloat();
    auto headerArea = bounds.removeFromTop (60.0f);

    // Subtle brushed metal texture / gradient
    juce::ColourGradient gradient (juce::Colour (0xFF2D2D2D), 0.0f, 0.0f,
                                  juce::Colour (0xFF121212), 0.0f, 400.0f, false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds.reduced (10.0f), 10.0f);

    // Header text
    g.setColour (juce::Colours::teal);
    g.setFont (juce::Font ("Arial", 32.0f, juce::Font::bold | juce::Font::italic));
    g.drawFittedText ("COSMIC RAYS", headerArea.toNearestInt(), juce::Justification::centred, 1);

    // Sections
    g.setColour (juce::Colours::white.withAlpha (0.1f));
    g.drawRoundedRectangle (bounds.reduced (20.0f), 8.0f, 1.0f);
}

void CosmicRaysAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (80); // Space for header
    
    auto bottomArea = area.removeFromBottom (100);
    auto topArea = area;

    int knobSize = 120;
    
    auto granularArea = topArea.removeFromLeft (area.getWidth() / 2);
    activitySlider.setBounds (granularArea.removeFromTop (knobSize).reduced (10));
    activityLabel.setBounds (activitySlider.getBounds().withY (activitySlider.getBottom() - 10).withHeight (20));
    
    shapeSlider.setBounds (granularArea.removeFromTop (knobSize).reduced (10));
    shapeLabel.setBounds (shapeSlider.getBounds().withY (shapeSlider.getBottom() - 10).withHeight (20));

    auto filterArea = topArea;
    filterSlider.setBounds (filterArea.removeFromTop (knobSize).reduced (10));
    filterLabel.setBounds (filterSlider.getBounds().withY (filterSlider.getBottom() - 10).withHeight (20));

    mixSlider.setBounds (filterArea.removeFromTop (knobSize).reduced (10));
    mixLabel.setBounds (mixSlider.getBounds().withY (mixSlider.getBottom() - 10).withHeight (20));

    auto algoArea = bottomArea.reduced (20, 30);
    algoBox.setBounds (algoArea.removeFromBottom (30));
    algoLabel.setBounds (algoBox.getBounds().withY (algoBox.getY() - 25).withHeight (20));
}
