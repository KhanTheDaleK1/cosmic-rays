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
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CosmicRaysAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "ACTIVITY", 1 }, "Activity", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SHAPE", 1 }, "Shape", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "FILTER", 1 }, "Filter", 20.0f, 20000.0f, 20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MIX", 1 }, "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ALGO", 1 }, "Algorithm", juce::StringArray { "Mosaic", "Glitch", "Warp", "Ghost" }, 0));

    return { params.begin(), params.end() };
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

void CosmicRaysAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CosmicRaysAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CosmicRaysAudioProcessor(); }
