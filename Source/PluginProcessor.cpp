/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OversamplingTestAudioProcessor::OversamplingTestAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    oversampling(this),
    // DSP
    vibrato(),
    wavefolder(),
    saturator(),
    // PARAMS
    apvts(*this, nullptr, "params", param::createParameters()),
    gainP(apvts.getRawParameterValue(param::getID(param::ID::Gain))),
    vibFreqP(apvts.getRawParameterValue(param::getID(param::ID::VibratoFreq))),
    vibDepthP(apvts.getRawParameterValue(param::getID(param::ID::VibratoDepth))),
    waveFolderDriveP(apvts.getRawParameterValue(param::getID(param::ID::WaveFolderDrive))),
    saturatorDriveP(apvts.getRawParameterValue(param::getID(param::ID::SaturatorDrive)))
#endif
{
    vibrato.resize(getTotalNumInputChannels());
}
    

OversamplingTestAudioProcessor::~OversamplingTestAudioProcessor()
{
}

//==============================================================================
const juce::String OversamplingTestAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OversamplingTestAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OversamplingTestAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OversamplingTestAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OversamplingTestAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OversamplingTestAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OversamplingTestAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OversamplingTestAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OversamplingTestAudioProcessor::getProgramName (int index)
{
    return {};
}

void OversamplingTestAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OversamplingTestAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampling.prepareToPlay(sampleRate, samplesPerBlock);
    sampleRate = oversampling.getSampleRateUpsampled();
    samplesPerBlock = oversampling.getBlockSizeUp();
    auto latency = oversampling.getLatency();

    for(auto& v: vibrato)
        v.prepareToPlay(sampleRate, samplesPerBlock);
    latency += vibrato[0].getLatency() / oversampling.getUpsamplingFactor();

    setLatencySamples(latency);
}

void OversamplingTestAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OversamplingTestAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void OversamplingTestAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto numSamples = buffer.getNumSamples();
    {
        const auto totalNumInputChannels = getTotalNumInputChannels();
        const auto totalNumOutputChannels = getTotalNumOutputChannels();
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, numSamples);
    }
    
    const auto numChannelsIn = getChannelCountOfBus(true, 0);
    const auto numChannelsOut = buffer.getNumChannels();

    // upsampling
    auto bufferUp = oversampling.upsample(buffer, numChannelsIn, numChannelsOut);
    if (bufferUp == nullptr)
        bufferUp = &buffer;
    numSamples = bufferUp->getNumSamples();
    auto samples = bufferUp->getArrayOfWritePointers();

    // non-linear processing
    const auto vibFreq = vibFreqP->load();
    const auto vibDepth = vibDepthP->load();
    for (auto ch = 0; ch < vibrato.size(); ++ch)
    {
        vibrato[ch].depth = vibDepth;
        vibrato[ch].setFrequency(vibFreq);
        vibrato[ch].process(samples[ch], numSamples);
    }
    wavefolder.setDrive(juce::Decibels::decibelsToGain(waveFolderDriveP->load()));
    wavefolder.processBlock(*bufferUp);
    saturator.setDrive(saturatorDriveP->load());
    saturator.processBlock(*bufferUp);
    
    // downsampling
    if(bufferUp != &buffer)
        oversampling.downsample(&buffer, numChannelsOut);

    const auto gainV = juce::Decibels::decibelsToGain(gainP->load());
    buffer.applyGain(gainV);
}

//==============================================================================
bool OversamplingTestAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OversamplingTestAudioProcessor::createEditor()
{
    return new OversamplingTestAudioProcessorEditor (*this);
}

//==============================================================================
void OversamplingTestAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.state;
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OversamplingTestAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OversamplingTestAudioProcessor();
}

void OversamplingTestAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) {

}