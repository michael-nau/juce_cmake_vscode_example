/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include "PluginEditor.h"

//==============================================================================
TestpluginAudioProcessor::TestpluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
}

TestpluginAudioProcessor::~TestpluginAudioProcessor() {}

//==============================================================================
const juce::String TestpluginAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool TestpluginAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool TestpluginAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool TestpluginAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double TestpluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int TestpluginAudioProcessor::getNumPrograms() {
  return 1;  // NB: some hosts don't cope very well if you tell them there are 0
             // programs, so this should be at least 1, even if you're not
             // really implementing programs.
}

int TestpluginAudioProcessor::getCurrentProgram() { return 0; }

void TestpluginAudioProcessor::setCurrentProgram(int index) {}

const juce::String TestpluginAudioProcessor::getProgramName(int index) {
  return {};
}

void TestpluginAudioProcessor::changeProgramName(int index,
                                                 const juce::String &newName) {}

//==============================================================================
void TestpluginAudioProcessor::prepareToPlay(double sampleRate,
                                             int samplesPerBlock) {
  // Use this method as the place to do any pre-playback
  // initialisation that you need..

  juce::dsp::ProcessSpec spec{};

  spec.maximumBlockSize = samplesPerBlock;

  spec.sampleRate = sampleRate;

  spec.numChannels = getTotalNumOutputChannels();

  leftChain.prepare(spec);
  rightChain.prepare(spec);

  auto chainSettings = getChainSettings(apvts);

  auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
      sampleRate, chainSettings.midFreq, chainSettings.midQuality,
      juce::Decibels::decibelsToGain(chainSettings.midGainInDecibels));

  // TODO: Add coefficients for low and high filters and implement toggle
  // buttons

  *leftChain.get<ChainPositions::Peak>().coefficients =
      *peakCoefficients;  // expects an idx, we create an enum - also need to
                          // dereference to get coeffients
  *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}

void TestpluginAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TestpluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
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

void TestpluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
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

  auto chainSettings = getChainSettings(apvts);

  auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
      getSampleRate(), chainSettings.midFreq, chainSettings.midQuality,
      juce::Decibels::decibelsToGain(chainSettings.midGainInDecibels));

  *leftChain.get<ChainPositions::Peak>().coefficients =
      *peakCoefficients;  // expects an idx, we create an enum - also need to
                          // dereference to get coeffients
  *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

  juce::dsp::AudioBlock<float> block(buffer);

  auto leftBlock = block.getSingleChannelBlock(0);
  auto rightBlock = block.getSingleChannelBlock(1);

  juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
  juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

  leftChain.process(leftContext);
  rightChain.process(rightContext);

  // This is the place where you'd normally do the guts of your plugin's
}

//==============================================================================
bool TestpluginAudioProcessor::hasEditor() const {
  return true;  // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *TestpluginAudioProcessor::createEditor() {
  return new GenericAudioProcessorEditor(*this);
}

//==============================================================================
void TestpluginAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  // You should use this method to store your parameters in the memory block.
  // You could do that either as raw data, or use the XML or ValueTree classes
  // as intermediaries to make it easy to save and load complex data.
}

void TestpluginAudioProcessor::setStateInformation(const void *data,
                                                   int sizeInBytes) {
  // You should use this method to restore your parameters from this memory
  // block, whose contents wil  l have been created by the getStateInformation()
  // call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts) {
  ChainSettings settings;
  settings.highCutFreq = apvts.getRawParameterValue("highFreq")->load();
  settings.highGainInDecibels = apvts.getRawParameterValue("highGain")->load();
  settings.highCutSlope = apvts.getRawParameterValue("highSlope")->load();

  settings.midFreq = apvts.getRawParameterValue("midFreq")->load();
  settings.midGainInDecibels = apvts.getRawParameterValue("midGain")->load();
  settings.midQuality = apvts.getRawParameterValue("midQ")->load();

  settings.lowCutFreq = apvts.getRawParameterValue("lowFreq")->load();
  settings.lowGainInDecibels = apvts.getRawParameterValue("lowGain")->load();
  settings.lowCutSlope = apvts.getRawParameterValue("lowSlope")->load();

  // toggle buttons
  settings.lowToggle = apvts.getRawParameterValue("lowToggle");
  settings.midToggle = apvts.getRawParameterValue("midToggle");
  settings.highToggle = apvts.getRawParameterValue("highToggle");

  return settings;
}

/* This is where we declare the parameters that we want to use in our plugin
 As we are making a EQ plugin we will need 3 parameters, one for each band (low,
 mid, high) we will also need a parameter for the following:
 1. gain of each band
 2. frequency of each band
 3. slope of the low and high bands
 4. 'quality' of the mid band i.e the bandwidth
 5.  master gain of the plugin
 6. band toggle buttons
*/
juce::AudioProcessorValueTreeState::ParameterLayout
TestpluginAudioProcessor::createParameters() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  float minGain = -24.0f;
  float maxGain = 24.0f;
  float gainStep = 0.1f;
  float freqStep = 1.0f;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "midFreq", "Mid Freq",
      juce::NormalisableRange<float>(200.0f, 2000.0f, freqStep), 200.0f, "Hz"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "midGain", "Mid Gain",
      juce::NormalisableRange<float>(minGain, maxGain, gainStep), 0.0f, "dB"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "midQ", "Mid Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 0.1f,
      ""));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lowFreq", "Low Freq",
      juce::NormalisableRange<float>(20.0f, 200.0f, freqStep), 20.0f, "Hz"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lowGain", "Low Gain",
      juce::NormalisableRange<float>(minGain, maxGain, gainStep), 0.0f, "dB"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "highFreq", "High Freq",
      juce::NormalisableRange<float>(2000.0f, 20000.0f, freqStep), 2000.0f,
      "Hz"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "highGain", "High Gain",
      juce::NormalisableRange<float>(minGain, maxGain, gainStep), 0.0f, "dB"));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "masterGain", "Master Gain",
      juce::NormalisableRange<float>(minGain, maxGain, gainStep), 0.0f, "dB"));

  // create a string array for the slope dropdown menu
  juce::StringArray stringArray;
  for (int i = 0; i < 10; i++) {
    juce::String str;
    str << (12 + i * 12) << " dB/oct";
    stringArray.add(str);
  }

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "lowSlope", "Low Slope", stringArray, 0.0f, ""));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "highSlope", "High Slope", stringArray, 0.0f, ""));

  layout.add(std::make_unique<juce::AudioParameterBool>(

      "lowToggle", "Low Toggle", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(

      "midToggle", "Mid Toggle", false));

  layout.add(std::make_unique<juce::AudioParameterBool>("highToggle",
                                                        "High Toggle", false));

  return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new TestpluginAudioProcessor();
}
