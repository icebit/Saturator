/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SaturatorAudioProcessor::SaturatorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "PluginState",
            {std::make_unique<juce::AudioParameterFloat>(
                 "inputGain",  // Parameter ID
                 "Input Gain", // Parameter name
                 juce::NormalisableRange<float>(-60.0f, 60.0f,
                                                0.1f), // Range and step size
                 30.0f                                 // Default value
                 ),
             std::make_unique<juce::AudioParameterFloat>(
                 "outputGain",  // Parameter ID
                 "Output Gain", // Parameter name
                 juce::NormalisableRange<float>(-60.0f, 60.0f,
                                                0.1f), // Range and step size
                 30.0f                                 // Default value
                 )})
#endif
{
  // DiodeClipper doesn't need configuration - it models the circuit directly

  auto &preGain = processorChain.template get<preGainIndex>();
  preGain.setGainDecibels(apvts.getParameterAsValue("inputGain")
                              .getValue()); // Initialize with APVTS value

  auto &postGain = processorChain.template get<postGainIndex>();
  postGain.setGainDecibels(apvts.getParameterAsValue("outputGain")
                               .getValue()); // Initialize with APVTS value
}

SaturatorAudioProcessor::~SaturatorAudioProcessor() {}

//==============================================================================
const juce::String SaturatorAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool SaturatorAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool SaturatorAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool SaturatorAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double SaturatorAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SaturatorAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int SaturatorAudioProcessor::getCurrentProgram() { return 0; }

void SaturatorAudioProcessor::setCurrentProgram(int index) {}

const juce::String SaturatorAudioProcessor::getProgramName(int index) {
  return {};
}

void SaturatorAudioProcessor::changeProgramName(int index,
                                                const juce::String &newName) {}

//==============================================================================
void SaturatorAudioProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialisation that you need..

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
  spec.numChannels = getTotalNumOutputChannels();

  processorChain.prepare(spec);
}

void SaturatorAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.

  processorChain.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SaturatorAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  // Some plugin hosts, such as certain GarageBand versions, will only
  // load plugins that support stereo bus layouts.
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

void SaturatorAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                           juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  /*
  // This is the place where you'd normally do the guts of your plugin's
  // audio processing...
  // Make sure to reset the state if your inner loop is processing
  // the samples and the outer loop is handling the channels.
  // Alternatively, you can process the samples with the channels
  // interleaved by keeping the same state.
  for (int channel = 0; channel < totalNumInputChannels; ++channel)
  {
      auto* channelData = buffer.getWritePointer (channel);

      // ..do something to the data...
  }*/

  // Update the pre-gain value from the APVTS in each block
  auto &preGain = processorChain.template get<preGainIndex>();
  preGain.setGainDecibels(apvts.getParameterAsValue("inputGain").getValue());

  auto &postGain = processorChain.template get<postGainIndex>();
  postGain.setGainDecibels(apvts.getParameterAsValue("outputGain").getValue());

  juce::dsp::AudioBlock<float> block(buffer);
  processorChain.process(juce::dsp::ProcessContextReplacing<float>(block));
}

//==============================================================================
bool SaturatorAudioProcessor::hasEditor() const {
  return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SaturatorAudioProcessor::createEditor() {
  return new SaturatorAudioProcessorEditor(*this);
}

//==============================================================================
void SaturatorAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  // Save the APVTS state
  std::unique_ptr<juce::XmlElement> xml(apvts.state.createXml());
  copyXmlToBinary(*xml, destData);
}

void SaturatorAudioProcessor::setStateInformation(const void *data,
                                                  int sizeInBytes) {
  // Restore the APVTS state
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr) {
    if (xmlState->hasTagName(apvts.state.getType())) {
      apvts.state = juce::ValueTree::fromXml(*xmlState);
    }
  }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new SaturatorAudioProcessor();
}
