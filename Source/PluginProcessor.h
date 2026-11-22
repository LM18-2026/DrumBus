#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
class DrumBusProcessor : public juce::AudioProcessor, private juce::Timer {
public:
 DrumBusProcessor();
 ~DrumBusProcessor() override {stopTimer();}
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout& b) const override {
  return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo() && b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
 }
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override {return true;}
 const juce::String getName() const override {return "LM18 DrumBus";}
 bool acceptsMidi() const override {return false;}
 bool producesMidi() const override {return false;}
 bool isMidiEffect() const override {return false;}
 double getTailLengthSeconds() const override {return 0.1;}
 int getNumPrograms() override {return 1;}
 int getCurrentProgram() override {return 0;}
 void setCurrentProgram(int) override {}
 const juce::String getProgramName(int) override {return {};}
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState state;
 std::atomic<int> analysisPercent{0};
 std::atomic<bool> matched{false};
private:
 static constexpr int bands=30,fftOrder=12,fftSize=1<<fftOrder;
 static juce::AudioProcessorValueTreeState::ParameterLayout params();
 void timerCallback() override;
 void analyseReference();
 void buildMatch();
 void analyseFrame(const float* samples,std::array<double,bands>& totals);
 int bandForFrequency(double hz) const;
 juce::dsp::FFT fft{fftOrder};
 std::array<double,bands> reference{},input{},frequencies{};
 std::array<float,fftSize> window{},frame{};
 std::array<float,fftSize*2> fftData{};
 std::array<std::array<juce::dsp::IIR::Filter<float>,bands>,2> eq;
 juce::AudioBuffer<float> captured;
 std::atomic<int> capturedCount{0};
 int captureSize=0;
 std::atomic<bool> readyToMatch{false};
 bool wasListening=false;
 juce::SpinLock eqLock;
 double rate=48000.0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusProcessor)
};
