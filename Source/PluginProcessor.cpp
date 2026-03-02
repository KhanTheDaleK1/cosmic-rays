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

CosmicRaysAudioProcessor::~CosmicRaysAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout CosmicRaysAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Granular Synthesis Macros (Fragments / Portal style)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "GRAIN_SIZE", 1 }, "Grain Size", 10.0f, 500.0f, 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "DENSITY", 1 }, "Density", 1.0f, 100.0f, 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "PITCH", 1 }, "Pitch", -24.0f, 24.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SPRAY", 1 }, "Pan Spray", 0.0f, 1.0f, 0.5f));

    // Delay & Tape Simulation (Other Desert Cities / Replicas style)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "DELAY_TIME", 1 }, "Delay Time", 1.0f, 2000.0f, 250.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "FEEDBACK", 1 }, "Feedback", 0.0f, 1.5f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "FLUTTER", 1 }, "Tape Flutter", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "REVERSE", 1 }, "Reverse Prob", 0.0f, 1.0f, 0.2f));

    // Master Output & Routing
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MIX", 1 }, "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ALGO", 1 }, "Algorithm", juce::StringArray { "Mosaic", "Glitch", "Warp", "Ghost" }, 0));

    return { params.begin(), params.end() };
}

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

void CosmicRaysAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    int numChannels = getTotalNumInputChannels() > 0 ? getTotalNumInputChannels() : 2;
    granularEngine.prepare(sampleRate, samplesPerBlock, numChannels);
}

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

    granularEngine.processBlock(buffer, apvts);
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