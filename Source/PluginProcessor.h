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
 const juce::String getName() const override {return "LM18 DRUMS";}
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
 std::atomic<float> compReductionDb{0.f},compGrVuDb{0.f};
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
 void restoreSavedMatch();
 void analyseFrame(const float* samples,std::array<double,bands>& totals);
 int bandForFrequency(double hz) const;
 juce::dsp::FFT fft{fftOrder};
 std::array<double,bands> reference{},input{},frequencies{};
 std::array<float,bands> savedMatchGains{};
 bool savedMatchValid=false;
 std::array<float,fftSize> window{},frame{};
 std::array<float,fftSize*2> fftData{};
 using EqBank=std::array<std::array<juce::dsp::IIR::Filter<float>,bands>,2>;
 std::array<EqBank,2> eqBanks;
 std::atomic<int> activeEqBank{0},pendingEqBank{-1};
 std::atomic<bool> eqCrossfading{false};
 int eqFadeRemaining=0,eqFadeTotal=1,eqFadeFrom=0,eqFadeTo=0;
 bool eqFadeFromDry=false;
 juce::AudioBuffer<float> captured,processed,reverbBuffer;
 juce::dsp::Convolution reverb;
 juce::SmoothedValue<float> reverbMix;
 float lastReverbLength=0.f;
 float eqLevelGain=1.f;
 // Measured RedEQ bass/treble curves: 14 minimum-phase peaking sections each.
 static constexpr int sweetBands=14;
 struct SweetBiquad {
  double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
  void setPeak(double sr,double hz,double q,double db) noexcept {
   const double w=juce::MathConstants<double>::twoPi*juce::jlimit(10.0,sr*0.45,hz)/sr;
   const double alpha=std::sin(w)/(2.0*q),cs=std::cos(w),A=std::pow(10.0,db/40.0);
   const double inv=1.0/(1.0+alpha/A);
   b0=(1.0+alpha*A)*inv;b1=-2.0*cs*inv;b2=(1.0-alpha*A)*inv;
   a1=-2.0*cs*inv;a2=(1.0-alpha/A)*inv;
  }
  float process(float x) noexcept {
   const double y=b0*x+z1;
   z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
   return (float)y;
  }
  void reset() noexcept {z1=z2=0.0;}
 };
 std::array<std::array<SweetBiquad,sweetBands>,2> sweetBass{},sweetTreble{};
 std::array<SweetBiquad,2> sweetParam{};
 juce::SmoothedValue<float> sweetBassSmooth,sweetTrebleSmooth,sweetGainSmooth,sweetSatSmooth;
 float sweetLastFreq=1000.f,sweetLastQ=1.f;

 std::array<float,2> tiltLowState{{0.f,0.f}};
 float tiltLowCoeff=0.f;
 juce::SmoothedValue<float> tiltGainSmooth;
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
 std::array<float,2> compEnvelope{{0.f,0.f}},compGainDb{{0.f,0.f}};
 float compStereoEnvelope=0.f,compStereoGainDb=0.f,compGrVuState=0.f;
 juce::SmoothedValue<float> compModeSmooth;
 juce::SmoothedValue<float> compEnableSmooth;
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
