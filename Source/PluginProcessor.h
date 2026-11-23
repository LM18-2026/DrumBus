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
 std::atomic<int> clipAnalysisPercent{0};
 std::atomic<bool> clipReady{false};
 std::atomic<bool> clipCaptureComplete{false};
 std::atomic<float> clipMakeup{1.f};
 std::atomic<float> inputPeakDb{-100.f},outputPeakDb{-100.f},inputRmsDb{-100.f},outputRmsDb{-100.f};
 std::atomic<float> inputVuDb{-100.f},outputVuDb{-100.f};
 void resetMeters() noexcept {metersResetGeneration.fetch_add(1,std::memory_order_release);}
 static constexpr int eqDisplayPoints=160;
 using EqResponse=std::array<float,eqDisplayPoints>;
 bool getEqResponse(EqResponse& response);
private:
 static constexpr int bands=30,fftOrder=12,fftSize=1<<fftOrder;
 static juce::AudioProcessorValueTreeState::ParameterLayout params();
 void timerCallback() override;
 void analyseReference();
 void loadScaledReverb(float lengthControl);
 void buildMatch();
 void calibrateClipper();
 void analyseFrame(const float* samples,std::array<double,bands>& totals);
 int bandForFrequency(double hz) const;
 juce::dsp::FFT fft{fftOrder};
 std::array<double,bands> reference{},input{},frequencies{};
 std::array<float,fftSize> window{},frame{};
 std::array<float,fftSize*2> fftData{};
 std::array<std::array<juce::dsp::IIR::Filter<float>,bands>,2> eq;
 juce::AudioBuffer<float> captured,processed,reverbBuffer,clipCapturedPre,clipCapturedOriginal;
 juce::dsp::Convolution reverb;
 juce::SmoothedValue<float> reverbMix;
 float lastReverbLength=0.f;
 float eqLevelGain=1.f;
 double rmsInputSum=0.0,rmsOutputSum=0.0;
 int rmsSamples=0;
 float rmsTrim=1.f;
 juce::SmoothedValue<float,juce::ValueSmoothingTypes::Multiplicative> rmsTrimSmooth;
 std::atomic<unsigned> metersResetGeneration{0};
 unsigned metersAppliedGeneration=0;
 double meterInputPower=0.0,meterOutputPower=0.0;
 double meterInputEnergy=0.0,meterOutputEnergy=0.0;
 juce::uint64 meterFrames=0;
 float meterInputMax=0.f,meterOutputMax=0.f;
 std::array<juce::dsp::IIR::Filter<float>,2> clipPre,clipPost,clipPostClean;
 float clipThreshold=1.f;
 float clipObservedPeak=0.f;
 float clipOriginalPeak=0.f;
 float clipCalibratedReduction=-1.f;
 int clipSamples=0;
 bool clipWasListening=false;
 juce::SmoothedValue<float> clipThresholdSmooth;
 std::atomic<int> capturedCount{0};
 int captureSize=0;
 std::atomic<bool> readyToMatch{false};
 float eqOutputGain=1.f;
 float lastStrength=35.f;
 bool wasListening=false;
 juce::SpinLock eqLock;
 double rate=48000.0;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusProcessor)
};
