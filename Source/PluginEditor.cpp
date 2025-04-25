/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SaturatorAudioProcessorEditor::SaturatorAudioProcessorEditor (SaturatorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
inputGainSliderAttachment (audioProcessor.apvts, "inputGain", inputGainSlider),
outputGainSliderAttachment (audioProcessor.apvts, "outputGain", outputGainSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    
    // Make sure that before the constructor has finished, you've added the
    // editor to the parent.
    inputGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    inputGainSlider.setRange (-60.0f, 60.0f, 0.1f);
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    inputGainSlider.setBounds(10, 10, 200, 30);
    addAndMakeVisible (inputGainSlider);
    
    outputGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    outputGainSlider.setRange (-60.0f, 60.0f, 0.1f);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    outputGainSlider.setBounds(10, 40, 200, 30);
    addAndMakeVisible (outputGainSlider);
    
    resized();
    
}

SaturatorAudioProcessorEditor::~SaturatorAudioProcessorEditor()
{
}

//==============================================================================
void SaturatorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SaturatorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    inputGainSlider.setBounds(10, 10, getWidth() - 20, 10);
    outputGainSlider.setBounds(10, 40, getWidth() - 20, 10);
}
