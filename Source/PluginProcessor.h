#pragma once

#include "Param.h"
#include "oversampling/Oversampling.h"
#include "NonLinearDSP.h"
#include <JuceHeader.h>

struct IDs {
    juce::Identifier fUFs = "fUFs";
    juce::Identifier fUC = "fUC";
    juce::Identifier fUBw = "fUBw";
    juce::Identifier fDFs = "fDFs";
    juce::Identifier fDC = "fDC";
    juce::Identifier fDBw = "fDBw";
};

class OversamplingTestAudioProcessor :
    public juce::AudioProcessor
{
public:
    //==============================================================================
    OversamplingTestAudioProcessor();
    ~OversamplingTestAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    oversampling::Processor oversampling;
    
    std::vector<dsp::Vibrato> vibrato;
    dsp::Wavefolder wavefolder;
    dsp::Saturator saturator;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> *gainP, *vibFreqP, *vibDepthP, *waveFolderDriveP, *saturatorDriveP;

    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OversamplingTestAudioProcessor)
};
