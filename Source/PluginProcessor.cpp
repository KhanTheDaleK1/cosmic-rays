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
       apvts (*this, nullptr, "Parameters", createParameterLayout()),
       visualiser(1)
#endif
{
}

CosmicRaysAudioProcessor::~CosmicRaysAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout CosmicRaysAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Category & Algorithm Selection
    juce::StringArray algos;
    algos.add("Mosaic"); algos.add("Seq"); algos.add("Glide");
    algos.add("Haze"); algos.add("Tunnel"); algos.add("Strum");
    algos.add("Blocks"); algos.add("Interrupt"); algos.add("Arp");
    algos.add("Pattern"); algos.add("Warp");
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ALGO", 1 }, "Algorithm", algos, 0));

    // Main Knobs
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "ACTIVITY", 1 }, "Activity", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "TIME", 1 }, "Time", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SHAPE", 1 }, "Shape", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "REPEATS", 1 }, "Repeats", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "FILTER", 1 }, "Filter", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SPACE", 1 }, "Space", 0.0f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MIX", 1 }, "Mix", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "LOOP_LEVEL", 1 }, "Loop Level", 0.0f, 1.0f, 0.8f));

    // Modulation Section (Wow & Flutter)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MOD_RATE", 1 }, "Mod Rate", 0.1f, 10.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "MOD_DEPTH", 1 }, "Mod Depth", 0.0f, 1.0f, 0.2f));

    // Global Modes
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "KILL_DRY", 1 }, "Kill Dry", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "TRAILS", 1 }, "Trails", true));

    // Advanced Controls
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "RESONANCE", 1 }, "Resonance", 0.0f, 1.0f, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "PITCH_DRIFT", 1 }, "Pitch Drift", 0.0f, 1.0f, 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "SNAP_STRENGTH", 1 }, "Rhythmic Snap", 0.0f, 1.0f, 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "TEMPO_MODE", 1 }, "Tempo Sync", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "FREEZE", 1 }, "Freeze/Sustain", false));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "SUBDIV", 1 }, "Subdivision", 
        juce::StringArray {"1/1", "1/2", "1/4", "1/8", "1/16"}, 2));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "GAIN", 1 }, "Output Gain", 0.0f, 2.0f, 1.0f));
    
    // 16 Presets (4 banks of 4)
    juce::StringArray presets;
    for (int b = 1; b <= 4; ++b) for (int s = 1; s <= 4; ++s) presets.add ("Bank " + juce::String(b) + " - " + juce::String(s));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "PRESET", 1 }, "Preset", presets, 0));

    // Phase Looper Controls
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "LOOPER_MODE", 1 }, "Looper Routing", 
        juce::StringArray { "Pre-FX", "Post-FX" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "LOOPER_REC", 1 }, "Looper Record", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "LOOPER_ODUB", 1 }, "Looper Overdub", false));
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
    visualiser.clear();
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
    visualiser.pushBuffer(buffer);
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