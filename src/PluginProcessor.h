/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct ChainSettings {
  float midFreq{0}, midGainInDecibels{0}, midQuality{1.f};
  float lowCutFreq{0}, highCutFreq{0};
  int lowCutSlope{0}, highCutSlope{0};
  float lowGainInDecibels{0}, highGainInDecibels{0}, masterGainInDecibels{0};
  bool lowToggle{true}, midToggle{true}, highToggle{true};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts);
//==============================================================================
/**
 */
class TestpluginAudioProcessor : public juce::AudioProcessor {
 public:
  //==============================================================================
  TestpluginAudioProcessor();
  ~TestpluginAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
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
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  // this is the function that creates the parameters | required by apvts below
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

  // need to declare audioProcessorValueTreeState here so we can attach the
  // parameters to it and display in the GUI it needs to be pulbic so the GUI
  // can see it
  juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters",
                                           createParameters()};

 private:
  using Filter = juce::dsp::IIR::Filter<float>;

  using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

  // declare a mono chain, representing one channel of audio (L or R)
  using MonoChain =
      juce::dsp::ProcessorChain<CutFilter, Filter,
                                CutFilter>;  // if implementing boost - need to
                                             // add it here

  // create 2 instances of the mono chain
  MonoChain leftChain, rightChain;

  enum ChainPositions {
    LowCut,
    Peak,
    HighCut
  };  // equivalent to the mono chain

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestpluginAudioProcessor)
};