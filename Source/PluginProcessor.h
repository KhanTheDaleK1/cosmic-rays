/*
  ==============================================================================
    Cosmic Rays - Granular Processor
    Copyright (C) 2026 Evan Beechem

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.fsf.org/licenses/>.
  ==============================================================================
*/
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "GranularEngine.h"

class CosmicRaysAudioProcessor  : public juce::AudioProcessor
{
public:
    CosmicRaysAudioProcessor();
    ~CosmicRaysAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    GranularEngine& getGranularEngine() { return granularEngine; }

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioVisualiserComponent visualiser;
    juce::UndoManager undoManager;
    juce::AudioProcessLoadMeasurer loadMeasurer;

    double getCPUUsage() { return loadMeasurer.getLoadAsPercentage(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    GranularEngine granularEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CosmicRaysAudioProcessor)
};