#include "PluginProcessor.h"
#include "PluginEditor.h"

CosmicRaysAudioProcessor::CosmicRaysAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

CosmicRaysAudioProcessor::~CosmicRaysAudioProcessor() {}

const juce::String CosmicRaysAudioProcessor::getName() const { return JucePlugin_Name; }

bool CosmicRaysAudioProcessor::acceptsMidi() const { return true; }
bool CosmicRaysAudioProcessor::producesMidi() const { return false; }
bool CosmicRaysAudioProcessor::isMidiEffect() const { return false; }
double CosmicRaysAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int CosmicRaysAudioProcessor::getNumPrograms() { return 1; }
int CosmicRaysAudioProcessor::getCurrentProgram() { return 0; }
void CosmicRaysAudioProcessor::setCurrentProgram (int index) {}
const juce::String CosmicRaysAudioProcessor::getProgramName (int index) { return {}; }
void CosmicRaysAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

void CosmicRaysAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void CosmicRaysAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CosmicRaysAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CosmicRaysAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Main Granular / Delay logic would go here.
    // For now, it's just a pass-through.
}

bool CosmicRaysAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* CosmicRaysAudioProcessor::createEditor() { return new CosmicRaysAudioProcessorEditor (*this); }

void CosmicRaysAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void CosmicRaysAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CosmicRaysAudioProcessor(); }
