#include "PluginProcessor.h"
#include "PluginEditor.h"

CosmicRaysAudioProcessorEditor::CosmicRaysAudioProcessorEditor (CosmicRaysAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (400, 300);
}

CosmicRaysAudioProcessorEditor::~CosmicRaysAudioProcessorEditor() {}

void CosmicRaysAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Cosmic Rays", getLocalBounds(), juce::Justification::centred, 1);
}

void CosmicRaysAudioProcessorEditor::resized() {}
