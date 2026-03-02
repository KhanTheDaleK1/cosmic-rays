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

    // Microcosm Category & Algorithm Selection
    juce::StringArray algos;
    algos.add("Mosaic (Micro Loop)"); algos.add("Seq (Micro Loop)"); algos.add("Glide (Micro Loop)");
    algos.add("Haze (Granules)"); algos.add("Tunnel (Granules)"); algos.add("Strum (Granules)");
    algos.add("Blocks (Glitch)"); algos.add("Interrupt (Glitch)"); algos.add("Arp (Glitch)");
    algos.add("Pattern (Multidelay)"); algos.add("Warp (Multidelay)");
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ALGO", 1 }, "Algorithm", algos, 0));

    // Main Knobs (Primary)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "ACTIVITY", 1 }, "Activity", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "TIME", 1 }, "Time", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SHAPE", 1 }, "Shape", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "REPEATS", 1 }, "Repeats", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "FILTER", 1 }, "Filter", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SPACE", 1 }, "Space", 0.0f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MIX", 1 }, "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "LOOP_LEVEL", 1 }, "Loop Level", 0.0f, 1.0f, 0.8f));

    // Secondary Controls (Accessed via Shift)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "RESONANCE", 1 }, "Resonance", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MOD_RATE", 1 }, "Mod Rate", 0.1f, 10.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MOD_DEPTH", 1 }, "Mod Depth", 0.0f, 1.0f, 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "SPACE_MODE", 1 }, "Space Mode", juce::StringArray {"Room", "Hall", "Ambient", "Wash"}, 0));

    // Logic & Toggles
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "TEMPO_MODE", 1 }, "Tempo Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "SUBDIV", 1 }, "Subdivision", 
        juce::StringArray {"1/1", "1/2", "1/4", "1/8", "1/16"}, 2));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "GAIN", 1 }, "Master Gain", 0.0f, 2.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "PRESET", 1 }, "Preset", juce::StringArray {"1", "2", "3", "4"}, 0));

    // Phase Looper Controls
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "LOOPER_MODE", 1 }, "Looper Mode", 
        juce::StringArray { "Pre-FX", "Looper Only", "Burst" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "LOOPER_QUANT", 1 }, "Quantize", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "LOOPER_REV", 1 }, "Reverse", false));

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