#include "PluginProcessor.h"
#include "PluginEditor.h"

OversamplingTestAudioProcessorEditor::OversamplingTestAudioProcessorEditor(OversamplingTestAudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    oversamplingEnabledButton(),

    gain(p, param::ID::Gain),
    vibratoFreq(p, param::ID::VibratoFreq),
    vibratoDepth(p, param::ID::VibratoDepth),
    wavefolderDrive(p, param::ID::WaveFolderDrive),
    saturatorDrive(p, param::ID::SaturatorDrive)
{
    addAndMakeVisible(oversamplingEnabledButton);
    oversamplingEnabledButton.name = "OverSampling\nEnabled";
    oversamplingEnabledButton.getState = [this]() {
        oversamplingEnabledButton.state = audioProcessor.oversampling.isEnabled();
        return oversamplingEnabledButton.state;
    };
    oversamplingEnabledButton.onClick = [this]() {
        oversamplingEnabledButton.state = !audioProcessor.oversampling.isEnabled();
        audioProcessor.oversampling.setEnabled(oversamplingEnabledButton.state);
    };
    oversamplingEnabledButton.getState();

    addAndMakeVisible(gain);
    addAndMakeVisible(vibratoFreq);
    addAndMakeVisible(vibratoDepth);
    addAndMakeVisible(wavefolderDrive);
    addAndMakeVisible(saturatorDrive);
    
    setOpaque(true);
    auto w = (int)p.apvts.state.getProperty("allWidth", 400);
    auto h = (int)p.apvts.state.getProperty("allHeight", 100);
    setSize (w, h);
}

//==============================================================================
void OversamplingTestAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void OversamplingTestAudioProcessorEditor::resized()
{
    auto x = 0;
    auto y = 0;
    auto w = getWidth();
    auto h = getHeight();
    auto wNum = w / 6;
    oversamplingEnabledButton.setBounds(x,y,wNum,h);
    x += wNum;
    wavefolderDrive.setBounds(x, y, wNum, h);
    x += wNum;
    saturatorDrive.setBounds(x, y, wNum, h);
    x += wNum;
    vibratoFreq.setBounds(x, y, wNum, h);
    x += wNum;
    vibratoDepth.setBounds(x, y, wNum, h);
    x += wNum;
    gain.setBounds(x, y, wNum, h);

    audioProcessor.apvts.state.setProperty("allWidth", getWidth(), nullptr);
    audioProcessor.apvts.state.setProperty("allHeight", getHeight(), nullptr);
}
