#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>
DrumBusProcessor::DrumBusProcessor()
 :juce::AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 state(*this,nullptr,"PARAMS",params()){
 for(int k=0;k<fftSize;++k)window[k]=0.5f-0.5f*std::cos(juce::MathConstants<float>::twoPi*(float)k/(fftSize-1));
 // The IR is loaded and amplitude-normalised in prepareToPlay.

 startTimerHz(10);
}
juce::AudioProcessorValueTreeState::ParameterLayout DrumBusProcessor::params(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"listen",1},"Analyse",false));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"enable",1},"Match EQ",true));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"blend",1},"EQ Blend",juce::NormalisableRange<float>(0.f,100.f,1.f),100.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"strength",1},"Match Depth",juce::NormalisableRange<float>(0.f,100.f,1.f),35.f));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"tiltEnable",1},"Tilt EQ Enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tiltDb",1},"Tilt dB",juce::NormalisableRange<float>(-12.f,12.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"reverbMix",1},"Reverb Dry Wet",juce::NormalisableRange<float>(0.f,100.f,1.f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"reverbLength",1},"Reverb IR Length",juce::NormalisableRange<float>(-100.f,100.f,1.f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reverbEnable",1},"Reverb Enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"compEnable",1},"FET Compressor Enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"compMode",1},"FET Channel Mode",juce::StringArray{"Stereo","Dual Mono"},0));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"compInput",1},"FET Input dB",juce::NormalisableRange<float>(-24.f,24.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"compOutput",1},"FET Output dB",juce::NormalisableRange<float>(-18.f,18.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"compAttack",1},"FET Attack ms",juce::NormalisableRange<float>(0.02f,0.8f,0.01f),0.3f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"compRelease",1},"FET Release ms",juce::NormalisableRange<float>(50.f,1100.f,1.f),200.f));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"compRatio",1},"FET Ratio",juce::StringArray{"4:1","8:1","12:1","20:1"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sweetEnable",1},"Sweetening EQ Enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sweetBass",1},"RedEQ Bass Percent",juce::NormalisableRange<float>(-100.f,100.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sweetTreble",1},"RedEQ Treble Percent",juce::NormalisableRange<float>(-100.f,100.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sweetFreq",1},"Sweet Param Hz",juce::NormalisableRange<float>(20.f,20000.f,0.1f,0.25f),1000.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sweetGain",1},"Sweet Param dB",juce::NormalisableRange<float>(-24.f,24.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sweetQ",1},"Sweet Param Q",juce::NormalisableRange<float>(0.1f,20.f,0.01f,0.35f),1.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sweetSat",1},"Sweet Param SAT",juce::NormalisableRange<float>(0.f,100.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"referenceType",1},"Reference",juce::StringArray{"KIT","KICK","SNARE"},0));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"matchOutput",1},"Match Output dB",juce::NormalisableRange<float>(-24.f,24.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"globalOutput",1},"Global Output dB",juce::NormalisableRange<float>(-24.f,24.f,0.1f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"globalBypass",1},"Global Bypass",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"moduleOrder",1},"Module Order",juce::StringArray{"Ambience > 1176","1176 > Ambience"},0));
 return {p.begin(),p.end()};
}
int DrumBusProcessor::bandForFrequency(double hz) const {
 if(hz<25.0||hz>20000.0)return -1;
 const double x=std::log(hz/25.0)/std::log(20000.0/25.0);
 return juce::jlimit(0,bands-1,(int)(x*bands));
}
void DrumBusProcessor::analyseFrame(const float* samples,std::array<double,bands>& totals){
 std::fill(fftData.begin(),fftData.end(),0.f);
 for(int i=0;i<fftSize;++i)fftData[i]=samples[i]*window[i];
 fft.performRealOnlyForwardTransform(fftData.data());
 std::array<int,bands> binCount{};
 std::array<double,bands> framePower{};
 for(int i=1;i<fftSize/2;++i){
  int b=bandForFrequency(i*rate/fftSize);
  if(b>=0){
   const double re=fftData[2*i],im=fftData[2*i+1];
   framePower[(size_t)b]+=(re*re+im*im);
   ++binCount[(size_t)b];
  }
 }
 // Average spectral power per FFT bin, not summed band energy.
 for(int b=0;b<bands;++b)if(binCount[(size_t)b]>0)
  totals[(size_t)b]+=framePower[(size_t)b]/binCount[(size_t)b];
}
void DrumBusProcessor::analyseReference(){
 juce::WavAudioFormat wav;
 auto* stream=new juce::MemoryInputStream(BinaryData::REFNoise1_wav,(size_t)BinaryData::REFNoise1_wavSize,false);
 std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(stream,true));
 reference.fill(0.0);
 if(!reader)return;
 juce::AudioBuffer<float> temp(2,fftSize);
 std::array<double,bands> sums{};
 int frames=0;
 for(juce::int64 offset=0;offset+fftSize<=reader->lengthInSamples;offset+=fftSize/2){
  temp.clear();
  reader->read(&temp,0,fftSize,offset,true,true);
  for(int i=0;i<fftSize;++i)frame[i]=temp.getSample(0,i);
  // Reference FFT must use the WAV sample rate, not the host sample rate.
  const double hostRate=rate;rate=reader->sampleRate;
  analyseFrame(frame.data(),sums);rate=hostRate;
  ++frames;
 }
 for(int b=0;b<bands;++b)reference[b]=sums[b]/juce::jmax(1,frames);
}
void DrumBusProcessor::prepareToPlay(double sr,int block){
 rate=sr;
 bypassDry.setSize(2,juce::jmax(1,block));
 for(auto* v:{&matchOutputSmooth,&globalOutputSmooth,&bypassSmooth})v->reset(sr,0.02);
 matchOutputSmooth.setCurrentAndTargetValue(1.f);globalOutputSmooth.setCurrentAndTargetValue(1.f);bypassSmooth.setCurrentAndTargetValue(0.f);
 captureSize=(int)std::ceil(sr*5.0);
 captured.setSize(1,captureSize);
 processed.setSize(2,juce::jmax(1,block));
 reverbBuffer.setSize(2,juce::jmax(1,block));
 reverb.prepare({sr,(juce::uint32)juce::jmax(1,block),2});
 reverbMix.reset(sr,0.03);
 reverbMix.setCurrentAndTargetValue(0.f);
 lastReverbLength=0.f;
 eqLevelGain=1.f;
 rmsInputSum=rmsOutputSum=0.0;rmsSamples=0;rmsTrim=1.f;
 rmsTrimSmooth.reset(sr,0.15);rmsTrimSmooth.setCurrentAndTargetValue(1.f);
 setLatencySamples(0);
 sweetBassSmooth.reset(sr,0.04);sweetTrebleSmooth.reset(sr,0.04);
 sweetGainSmooth.reset(sr,0.04);sweetSatSmooth.reset(sr,0.04);
 sweetBassSmooth.setCurrentAndTargetValue(0.f);sweetTrebleSmooth.setCurrentAndTargetValue(0.f);
 sweetGainSmooth.setCurrentAndTargetValue(0.f);sweetSatSmooth.setCurrentAndTargetValue(0.f);
 for(auto& ch:sweetBass)for(auto& f:ch)f.reset();
 for(auto& ch:sweetTreble)for(auto& f:ch)f.reset();
 for(auto& f:sweetParam)f.reset();
 tiltLowState.fill(0.f);
 tiltGainSmooth.reset(sr,0.025);tiltGainSmooth.setCurrentAndTargetValue(0.f);
 tiltLowCoeff=(float)std::exp(-juce::MathConstants<double>::twoPi*1000.0/sr);
 compEnvelope.fill(0.f);compGainDb.fill(0.f);compStereoEnvelope=0.f;compStereoGainDb=0.f;
 compGrVuState=0.f;compReductionDb.store(0.f);compGrVuDb.store(0.f);
 compModeSmooth.reset(sr,0.03);
 compModeSmooth.setCurrentAndTargetValue(state.getRawParameterValue("compMode")->load()>0.5f?1.f:0.f);
 compEnableSmooth.reset(sr,0.02);compEnableSmooth.setCurrentAndTargetValue(0.f);
 loadScaledReverb(0.f);
 captured.clear();
 capturedCount.store(0);
 readyToMatch.store(false);
 matched.store(false);
 analysisPercent.store(0);
 wasListening=false;
 lastStrength=*state.getRawParameterValue("strength");
 for(auto& bank:eqBanks)for(auto& channel:bank)for(auto& filter:channel)filter.reset();
 activeEqBank.store(0);pendingEqBank.store(-1);eqCrossfading.store(false);eqFadeRemaining=0;
 eqFadeTotal=juce::jmax(1,(int)std::round(sr*0.05));
 analyseReference();
 restoreSavedMatch();
}
void DrumBusProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals nd;
 const int n=buffer.getNumSamples();
 if(buffer.getNumChannels()<2)return;
 const bool canBypass=n<=bypassDry.getNumSamples();
 if(canBypass)for(int c=0;c<2;++c)bypassDry.copyFrom(c,0,buffer,c,0,n);
 const bool listening=*state.getRawParameterValue("listen")>0.5f;
 double inputPowerBlock=0.0,inputPeakBlock=0.0;
 for(int c=0;c<2;++c){const auto* d=buffer.getReadPointer(c);for(int i=0;i<n;++i){inputPowerBlock+=(double)d[i]*d[i];inputPeakBlock=juce::jmax(inputPeakBlock,(double)std::abs(d[i]));}}
 const double inputRmsBlock=std::sqrt(inputPowerBlock/juce::jmax(1,2*n));
 // Meter values are updated after the complete block; input is captured here.
 if(listening && !wasListening){
  // A new session always discards the previous EQ before recording.
  matched.store(false,std::memory_order_release);
  capturedCount.store(0,std::memory_order_release);
  rmsInputSum=rmsOutputSum=0.0;rmsSamples=0;
  rmsTrim=1.f;rmsTrimSmooth.setTargetValue(1.f);
  analysisPercent.store(0);
  readyToMatch.store(false,std::memory_order_release);
 }
 wasListening=listening;
 if(listening && rmsSamples<captureSize){
  const int count=juce::jmin(n,captureSize-rmsSamples);
  for(int c=0;c<2;++c){const auto* d=buffer.getReadPointer(c);for(int i=0;i<count;++i)rmsInputSum+=(double)d[i]*d[i];}
 }
 if(listening && !readyToMatch.load(std::memory_order_acquire)){
  int start=capturedCount.load(std::memory_order_relaxed);
  if(start<captureSize){
   const int count=juce::jmin(n,captureSize-start);
   auto* dest=captured.getWritePointer(0)+start;
   auto* l=buffer.getReadPointer(0);
   auto* r=buffer.getReadPointer(1);
   for(int i=0;i<count;++i)dest[i]=0.5f*(l[i]+r[i]);
   capturedCount.store(start+count,std::memory_order_release);
   analysisPercent.store((int)(100.0*(start+count)/captureSize));
   if(start+count>=captureSize)readyToMatch.store(true,std::memory_order_release);
  }
 }
 const float blend=juce::jlimit(0.f,1.f,*state.getRawParameterValue("blend")/100.f);
 const bool eqActive=blend>0.f && *state.getRawParameterValue("enable")>0.5f && matched.load(std::memory_order_acquire);
 // All coefficient updates happen in an inactive bank on the message thread.
 // The audio thread owns filter states and crossfades for 50 ms.
 const int requested=pendingEqBank.exchange(-1,std::memory_order_acq_rel);
 if(requested>=0){
  eqFadeFrom=activeEqBank.load(std::memory_order_relaxed);
  eqFadeTo=requested;
  eqFadeFromDry=!matched.load(std::memory_order_acquire);
  eqFadeRemaining=eqFadeTotal;
  eqCrossfading.store(true,std::memory_order_release);
  matched.store(true,std::memory_order_release);
 }
 if(n<=processed.getNumSamples() && (eqActive||eqFadeRemaining>0)){
  for(int i=0;i<n;++i){
   const bool fading=eqFadeRemaining>0;
   const float cross=fading ? 1.f-(float)eqFadeRemaining/(float)eqFadeTotal : 1.f;
   const int active=activeEqBank.load(std::memory_order_relaxed);
   for(int c=0;c<2;++c){
    auto* data=buffer.getWritePointer(c);
    const float dry=data[i];
    float wet=dry;
    if(fading){
     float from=dry,to=dry;
     if(!eqFadeFromDry)for(auto& filter:eqBanks[(size_t)eqFadeFrom][(size_t)c])from=filter.processSample(from);
     for(auto& filter:eqBanks[(size_t)eqFadeTo][(size_t)c])to=filter.processSample(to);
     wet=from+(to-from)*cross;
    }else if(eqActive){
     for(auto& filter:eqBanks[(size_t)active][(size_t)c])wet=filter.processSample(wet);
    }
    data[i]=dry*(1.f-blend)+wet*blend;
   }
   if(fading && --eqFadeRemaining==0){
    activeEqBank.store(eqFadeTo,std::memory_order_release);
    eqCrossfading.store(false,std::memory_order_release);
   }
  }
 }
 // Independent post-Match tilt EQ, first-order complementary low/high split.
 // At 0 dB both paths sum exactly to the unprocessed signal.
 const bool tiltOn=state.getRawParameterValue("tiltEnable")->load()>0.5f;
 const float tiltTarget=tiltOn?state.getRawParameterValue("tiltDb")->load():0.f;
 tiltGainSmooth.setTargetValue(tiltTarget);
 for(int i=0;i<n;++i){
  const float db=tiltGainSmooth.getNextValue();
  const float lowGain=juce::Decibels::decibelsToGain(-db);
  const float highGain=juce::Decibels::decibelsToGain(db);
  for(int c=0;c<2;++c){
   const float x=buffer.getSample(c,i);
   const float low=tiltLowCoeff*tiltLowState[(size_t)c]+(1.f-tiltLowCoeff)*x;
   tiltLowState[(size_t)c]=low;
   buffer.setSample(c,i,low*lowGain+(x-low)*highGain);
  }
 }
 // Compare the same five-second input window with the complete DSP output.
 if(listening && rmsSamples<captureSize){
  const int count=juce::jmin(n,captureSize-rmsSamples);
  for(int c=0;c<2;++c){const auto* d=buffer.getReadPointer(c);for(int i=0;i<count;++i)rmsOutputSum+=(double)d[i]*d[i];}
  rmsSamples+=count;
  if(rmsSamples>=captureSize && rmsInputSum>1.e-12 && rmsOutputSum>1.e-12){
   rmsTrim=juce::jlimit(0.25f,4.f,(float)std::sqrt(rmsInputSum/rmsOutputSum));
   rmsTrimSmooth.setTargetValue(rmsTrim);
  }
 }
 for(int i=0;i<n;++i){
  const float gain=rmsTrimSmooth.getNextValue();
  for(int c=0;c<2;++c)buffer.setSample(c,i,buffer.getSample(c,i)*gain);
 }
 matchOutputSmooth.setTargetValue(juce::Decibels::decibelsToGain(state.getRawParameterValue("matchOutput")->load()));
 for(int i=0;i<n;++i){const float g=matchOutputSmooth.getNextValue();for(int c=0;c<2;++c)buffer.setSample(c,i,buffer.getSample(c,i)*g);}
 auto runAmbience=[&](){
 // Feed convolution every block to preserve tails and continuity.
 if(n<=reverbBuffer.getNumSamples()){
  for(int c=0;c<2;++c)reverbBuffer.copyFrom(c,0,buffer,c,0,n);
  juce::dsp::AudioBlock<float> wetBlock(reverbBuffer);
  auto wetSubBlock=wetBlock.getSubBlock(0,(size_t)n);
  juce::dsp::ProcessContextReplacing<float> wetContext(wetSubBlock);
  reverb.process(wetContext);
  const bool enabled=*state.getRawParameterValue("reverbEnable")>0.5f;
  reverbMix.setTargetValue(enabled?*state.getRawParameterValue("reverbMix")/100.f:0.f);
  for(int i=0;i<n;++i){
   const float w=reverbMix.getNextValue();
   for(int c=0;c<2;++c){
    auto* dst=buffer.getWritePointer(c);
    dst[i]+=w*reverbBuffer.getSample(c,i);
   }
  }
 }

 };
 auto runCompressor=[&](){
 // Two independent detectors run continuously alongside the stereo-linked
 // detector. Crossfade their gain laws over 30 ms when changing modes.
 const bool compOn=state.getRawParameterValue("compEnable")->load()>0.5f;
 const bool dualMono=state.getRawParameterValue("compMode")->load()>0.5f;
 const float inputGain=juce::Decibels::decibelsToGain(state.getRawParameterValue("compInput")->load());
 const float outputGain=juce::Decibels::decibelsToGain(state.getRawParameterValue("compOutput")->load());
 const float attackMs=juce::jlimit(0.02f,0.8f,state.getRawParameterValue("compAttack")->load());
 const float releaseMs=juce::jlimit(50.f,1100.f,state.getRawParameterValue("compRelease")->load());
 const int ratioIndex=juce::jlimit(0,3,(int)state.getRawParameterValue("compRatio")->load());
 constexpr float ratios[]={4.f,8.f,12.f,20.f};
 const float ratio=ratios[ratioIndex];
 const float attackCoeff=std::exp(-1.f/(float)(rate*attackMs*0.001));
 const float releaseCoeff=std::exp(-1.f/(float)(rate*releaseMs*0.001));
 const float grVuAttack=std::exp(-1.f/(float)(rate*0.3));
 const float grVuRelease=std::exp(-1.f/(float)(rate*0.5));
 const auto updateDetector=[&](float detector,float& envelope,float& gainReduction){
  const float coeff=detector>envelope?attackCoeff:releaseCoeff;
  envelope=coeff*envelope+(1.f-coeff)*detector;
  const float levelDb=juce::Decibels::gainToDecibels(juce::jmax(envelope,1.e-8f),-160.f);
  const float over=levelDb+18.f;
  float targetReduction=0.f;
  if(over>1.5f)targetReduction=over*(1.f-1.f/ratio);
  else if(over>-1.5f){const float x=over+1.5f;targetReduction=x*x*(1.f-1.f/ratio)/6.f;}
  const float gainCoeff=targetReduction>gainReduction?attackCoeff:releaseCoeff;
  gainReduction=gainCoeff*gainReduction+(1.f-gainCoeff)*targetReduction;
 };
 compEnableSmooth.setTargetValue(compOn?1.f:0.f);
 compModeSmooth.setTargetValue(dualMono?1.f:0.f);
 float displayGr=0.f;
 for(int i=0;i<n;++i){
  const float l=buffer.getSample(0,i),r=buffer.getSample(1,i);
  const float drivenL=l*inputGain,drivenR=r*inputGain;
  updateDetector(juce::jmax(std::abs(drivenL),std::abs(drivenR)),compStereoEnvelope,compStereoGainDb);
  updateDetector(std::abs(drivenL),compEnvelope[0],compGainDb[0]);
  updateDetector(std::abs(drivenR),compEnvelope[1],compGainDb[1]);
  const float modeMix=compModeSmooth.getNextValue();
  const float grL=compStereoGainDb+(compGainDb[0]-compStereoGainDb)*modeMix;
  const float grR=compStereoGainDb+(compGainDb[1]-compStereoGainDb)*modeMix;
  const float wetL=drivenL*juce::Decibels::decibelsToGain(-grL)*outputGain;
  const float wetR=drivenR*juce::Decibels::decibelsToGain(-grR)*outputGain;
  const float mix=compEnableSmooth.getNextValue();
  buffer.setSample(0,i,l+(wetL-l)*mix);
  buffer.setSample(1,i,r+(wetR-r)*mix);
  // In dual mono the single GR display shows the greater reduction.
  displayGr=juce::jmax(grL,grR)*mix;
  const float grCoeff=displayGr>compGrVuState?grVuAttack:grVuRelease;
  compGrVuState=grCoeff*compGrVuState+(1.f-grCoeff)*displayGr;
 }
 compReductionDb.store(displayGr,std::memory_order_relaxed);
 compGrVuDb.store(compGrVuState,std::memory_order_relaxed);

 };
 if(state.getRawParameterValue("moduleOrder")->load()>0.5f){runCompressor();runAmbience();}else{runAmbience();runCompressor();}
 // Sweetening EQ: minimum-phase IIR approximation of measured RedEQ
 // maximum-boost impulse responses (44.1 kHz reference).
 // Gain values are logarithmically scaled, negative controls invert dB curves.
 // Each biquad retains state; coefficients are updated without allocations.
 static constexpr double centers[sweetBands]={
  30.,48.9,79.6,129.6,211.,343.6,559.6,911.3,
  1484.1,2417.,3936.1,6410.,10438.9,17000.};
 static constexpr double bassCurve[sweetBands]={
  15.390,3.432,6.616,3.129,3.160,1.151,0.462,-0.377,
  -0.459,-0.489,-0.679,-0.738,-1.822,-6.612};
 static constexpr double trebleCurve[sweetBands]={
  -0.017,0.145,0.058,0.085,0.043,-0.094,-0.239,-0.263,
  -0.109,0.317,1.340,3.518,7.565,6.789};
 const bool sweetOn=state.getRawParameterValue("sweetEnable")->load()>0.5f;
 sweetBassSmooth.setTargetValue(sweetOn?state.getRawParameterValue("sweetBass")->load()/100.f:0.f);
 sweetTrebleSmooth.setTargetValue(sweetOn?state.getRawParameterValue("sweetTreble")->load()/100.f:0.f);
 sweetGainSmooth.setTargetValue(sweetOn?state.getRawParameterValue("sweetGain")->load():0.f);
 sweetSatSmooth.setTargetValue(sweetOn?state.getRawParameterValue("sweetSat")->load()/100.f:0.f);
 const float paramFreq=state.getRawParameterValue("sweetFreq")->load();
 const float paramQ=state.getRawParameterValue("sweetQ")->load();
 for(int i=0;i<n;++i){
  const float bass=sweetBassSmooth.getNextValue(),treble=sweetTrebleSmooth.getNextValue();
  const float gain=sweetGainSmooth.getNextValue(),sat=sweetSatSmooth.getNextValue();
  // 32-sample coefficient ramp; filter states never reset during automation.
  if((i&31)==0){
   for(int c=0;c<2;++c){
    for(int b=0;b<sweetBands;++b){
     sweetBass[(size_t)c][(size_t)b].setPeak(rate,centers[b],0.8,bassCurve[b]*bass);
     sweetTreble[(size_t)c][(size_t)b].setPeak(rate,centers[b],0.8,trebleCurve[b]*treble);
    }
    sweetParam[(size_t)c].setPeak(rate,paramFreq,paramQ,gain);
   }
  }
  for(int c=0;c<2;++c){
   float x=buffer.getSample(c,i);
   for(auto& f:sweetBass[(size_t)c])x=f.process(x);
   for(auto& f:sweetTreble[(size_t)c])x=f.process(x);
   x=sweetParam[(size_t)c].process(x);
   // Soft saturation; 0% SAT is bit-identical to the unsaturated signal.
   if(sat>0.f){const float driven=std::tanh(2.5f*x)/std::tanh(2.5f);x+=sat*(driven-x);}
   buffer.setSample(c,i,x);
  }
 }
 globalOutputSmooth.setTargetValue(juce::Decibels::decibelsToGain(state.getRawParameterValue("globalOutput")->load()));
 bypassSmooth.setTargetValue(state.getRawParameterValue("globalBypass")->load()>0.5f?1.f:0.f);
 for(int i=0;i<n;++i){const float g=globalOutputSmooth.getNextValue(),b=bypassSmooth.getNextValue();for(int c=0;c<2;++c){const float wet=buffer.getSample(c,i);buffer.setSample(c,i,g*(canBypass?wet+(bypassDry.getSample(c,i)-wet)*b:wet));}}
 // VU-style RMS bar: 300 ms power-domain integration.
 // Numeric RMS: cumulative energy average since RESET.
 // Numeric PEAK: maximum absolute sample since RESET.
 const auto reset=metersResetGeneration.load(std::memory_order_acquire);
 if(reset!=metersAppliedGeneration){
  metersAppliedGeneration=reset;
  meterInputPower=0.0;meterOutputPower=0.0;
  meterInputEnergy=0.0;meterOutputEnergy=0.0;meterFrames=0;
  meterInputMax=0.f;meterOutputMax=0.f;
 }
 double outputPowerBlock=0.0,outputPeakBlock=0.0;
 for(int c=0;c<2;++c){
  const auto* d=buffer.getReadPointer(c);
  for(int i=0;i<n;++i){
   outputPowerBlock+=(double)d[i]*d[i];
   outputPeakBlock=juce::jmax(outputPeakBlock,(double)std::abs(d[i]));
  }
 }
 const double alpha=std::exp(-(double)n/(rate*0.3));
 const double inPower=inputPowerBlock/juce::jmax(1,2*n);
 const double outPower=outputPowerBlock/juce::jmax(1,2*n);
 meterInputPower=alpha*meterInputPower+(1.0-alpha)*inPower;
 meterOutputPower=alpha*meterOutputPower+(1.0-alpha)*outPower;
 inputVuDb.store(juce::Decibels::gainToDecibels((float)std::sqrt(meterInputPower),-100.f));
 outputVuDb.store(juce::Decibels::gainToDecibels((float)std::sqrt(meterOutputPower),-100.f));
 meterInputEnergy+=inputPowerBlock;meterOutputEnergy+=outputPowerBlock;
 meterFrames+=(juce::uint64)n;
 meterInputMax=juce::jmax(meterInputMax,(float)inputPeakBlock);
 meterOutputMax=juce::jmax(meterOutputMax,(float)outputPeakBlock);
 const double denom=juce::jmax(1.0,2.0*(double)meterFrames);
 inputPeakDb.store(juce::Decibels::gainToDecibels(meterInputMax,-100.f));
 outputPeakDb.store(juce::Decibels::gainToDecibels(meterOutputMax,-100.f));
 inputRmsDb.store(juce::Decibels::gainToDecibels((float)std::sqrt(meterInputEnergy/denom),-100.f));
 outputRmsDb.store(juce::Decibels::gainToDecibels((float)std::sqrt(meterOutputEnergy/denom),-100.f));
}
void DrumBusProcessor::loadScaledReverb(float amount){
 const double factor=amount<0.f?1.0+0.006*(double)amount:1.0+0.01*(double)amount;
 juce::WavAudioFormat wav;
 auto stream=std::make_unique<juce::MemoryInputStream>(BinaryData::LexRoom1_wav,(size_t)BinaryData::LexRoom1_wavSize,false);
 std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(stream.release(),true));
 if(!reader||reader->lengthInSamples<2||reader->numChannels<1)return;
 const int original=(int)reader->lengthInSamples;
 const int channels=juce::jmin(2,(int)reader->numChannels);
 juce::AudioBuffer<float> source(channels,original);
 if(!reader->read(&source,0,original,0,true,true))return;
 const int length=juce::jmax(2,(int)std::lround(original*factor));
 juce::AudioBuffer<float> scaled(channels,length);
 for(int c=0;c<channels;++c){
  const float* in=source.getReadPointer(c);
  float* out=scaled.getWritePointer(c);
  for(int i=0;i<length;++i){
   const double at=(double)i/factor;
   const int a=juce::jlimit(0,original-1,(int)at);
   const int b=juce::jmin(a+1,original-1);
   out[i]=in[a]+(in[b]-in[a])*(float)(at-a);
  }
 }
 // Normalize the IR by its L2 energy instead of the sum of absolute taps.
 // The 0.35 target is deliberately conservative and limits excessive wet level.
 double maxEnergy=0.0;
 for(int c=0;c<channels;++c){
  double energy=0.0;
  const float* taps=scaled.getReadPointer(c);
  for(int i=0;i<length;++i)energy+=(double)taps[i]*(double)taps[i];
  maxEnergy=juce::jmax(maxEnergy,energy);
 }
 if(maxEnergy>1.e-18){
  const float scale=(float)juce::jmin(4.0,0.35/std::sqrt(maxEnergy));
  scaled.applyGain(scale);
 }
 reverb.loadImpulseResponse(std::move(scaled),reader->sampleRate,
  channels==2?juce::dsp::Convolution::Stereo::yes:juce::dsp::Convolution::Stereo::no,
  juce::dsp::Convolution::Trim::no,juce::dsp::Convolution::Normalise::no);
}
void DrumBusProcessor::timerCallback(){
 const float length=*state.getRawParameterValue("reverbLength");
 if(length!=lastReverbLength){loadScaledReverb(length);lastReverbLength=length;}

 if(!readyToMatch.load(std::memory_order_acquire) && (eqCrossfading.load(std::memory_order_acquire)||pendingEqBank.load(std::memory_order_acquire)>=0))return;
 if(!readyToMatch.exchange(false)){
  const float strength=*state.getRawParameterValue("strength");
  if(matched.load(std::memory_order_acquire) && strength!=lastStrength){
   lastStrength=strength;
   buildMatch();
  }
  return;
 }
 lastStrength=*state.getRawParameterValue("strength");
 buildMatch();
 // Toggle analyse off through the host-visible parameter; next listen activation
 // should reset the capture explicitly in a later revision.
 if(auto* p=state.getParameter("listen"))p->setValueNotifyingHost(0.f);
}
void DrumBusProcessor::buildMatch(){
 if(eqCrossfading.load(std::memory_order_acquire)||pendingEqBank.load(std::memory_order_acquire)>=0)return;
 input.fill(0.0);
 int frames=0;
 const int total=capturedCount.load(std::memory_order_acquire);
 for(int offset=0;offset+fftSize<=total;offset+=fftSize/2){
  analyseFrame(captured.getReadPointer(0)+offset,input);
  ++frames;
 }
 if(frames==0)return;
 // Normalize log spectra to their mean shape. Reject bands with no signal.
 std::array<double,bands> difference{};
 double meanDifference=0.0;
 int valid=0;
 for(int b=0;b<bands;++b){
  static constexpr double kick[bands]={0,23012.9065,32819.5178,35458.2654,26686.1657,11125.1213,3256.435,743.716831,141.200481,10.348433,0.722451404,0.0844357813,0.186910321,0.209860175,0.0959276594,0.30783821,1.65221533,1.80850578,1.18414835,2.32944816,2.58425288,3.29470917,2.30675385,1.99919547,0.781651875,0.236490893,0.0746061171,0.0110440436,0.000756105168,0.000180178174};
 static constexpr double snare[bands]={0,113.624411,73.7842233,54.5881284,109.748497,151.193458,256.216145,1815.51756,9812.83495,25163.8605,5565.03947,725.568563,390.170151,92.4212617,34.2767278,53.7171839,56.772181,39.9345564,23.0461627,18.9568403,24.5689212,39.8536711,38.694818,36.03272,25.4426527,11.6317847,3.90032822,1.3724013,0.270843731,0.0251365024};
 const int refType=(int)state.getRawParameterValue("referenceType")->load();
 const double ref=refType==1?kick[b]:refType==2?snare[b]:reference[(size_t)b];
  const double in=input[(size_t)b]/frames;
  if(ref>1.e-12 && in>1.e-12){
   difference[(size_t)b]=10.0*std::log10(ref/in);
   meanDifference+=difference[(size_t)b];
   ++valid;
  }
 }
 if(valid<10){matched.store(false);return;}
 meanDifference/=valid;
 for(int b=0;b<bands;++b)difference[(size_t)b]-=meanDifference;
 // Five-band triangular smoothing avoids sharp spectral spikes.
 std::array<float,bands> gains{};
 for(int b=0;b<bands;++b){
  double weighted=0.0,weights=0.0;
  for(int d=-2;d<=2;++d){
   const int j=b+d;
   if(j>=0 && j<bands){
    const double w=3-std::abs(d);
    weighted+=w*difference[(size_t)j];
    weights+=w;
   }
  }
  // User-controlled match depth: default 35% reproduces V0.6 except where V0.6 capped at 12 dB.
  const float depth=juce::jlimit(0.f,1.f,*state.getRawParameterValue("strength")/100.f);
  gains[(size_t)b]=juce::jlimit(-30.f,30.f,(float)(depth*weighted/weights));
 }
 // Do not subtract the largest narrow-band boost from the entire signal.
 // Keep unity output trim; the host must retain sufficient headroom.
 const float newOutputGain=1.f;
 const int next=1-activeEqBank.load(std::memory_order_acquire);
 // The inactive bank is not accessed by the audio thread outside a fade.
 juce::SpinLock::ScopedLockType lock(eqLock);
 for(int b=0;b<bands;++b){
  const double f=25.0*std::pow(20000.0/25.0,(b+0.5)/(double)bands);
  frequencies[b]=f;
  const float center=(float)juce::jmin(f,rate*0.45);
  auto coeff=juce::dsp::IIR::Coefficients<float>::makePeakFilter(
   rate,center,4.0f,juce::Decibels::decibelsToGain(gains[(size_t)b]));
  for(int c=0;c<2;++c)*eqBanks[(size_t)next][(size_t)c][(size_t)b].coefficients=*coeff;
 }
 for(auto& channel:eqBanks[(size_t)next])for(auto& filter:channel)filter.reset();
 eqOutputGain=newOutputGain;
 savedMatchGains=gains;
 savedMatchValid=true;
 pendingEqBank.store(next,std::memory_order_release);
}

bool DrumBusProcessor::getEqResponse(EqResponse& response){
 if(!matched.load(std::memory_order_acquire)){
  response.fill(0.f);
  return true;
 }
 juce::SpinLock::ScopedTryLockType lock(eqLock);
 // Never overwrite the last valid curve if the EQ is being rebuilt.
 if(!lock.isLocked())return false;
 const bool enabled=state.getRawParameterValue("enable")->load()>0.5f;
 const double blend=juce::jlimit(0.0,1.0,(double)state.getRawParameterValue("blend")->load()/100.0);
 if(!enabled || blend<=0.0){
  response.fill(0.f);
  return true;
 }
 EqResponse nextResponse{};
 const double nyquist=rate*0.5;
 for(int i=0;i<eqDisplayPoints;++i){
  const double hz=20.0*std::pow(1000.0,(double)i/(eqDisplayPoints-1));
  if(hz>=nyquist){nextResponse[(size_t)i]=0.f;continue;}
  double magnitude=1.0;
  for(const auto& filter:eqBanks[(size_t)activeEqBank.load(std::memory_order_acquire)][0]){
   magnitude*=filter.coefficients->getMagnitudeForFrequency(hz,rate);
  }
  // Magnitude of dry/wet mixing, excluding time-varying RMS level trim.
  const double mixed=(1.0-blend)+blend*magnitude;
  nextResponse[(size_t)i]=(float)juce::jlimit(-60.0,60.0,20.0*std::log10(juce::jmax(1.e-9,mixed)));
 }
 response=nextResponse;
 return true;
}
void DrumBusProcessor::restoreSavedMatch(){
 juce::SpinLock::ScopedLockType lock(eqLock);
 if(!savedMatchValid)return;
 // Prepare the inactive bank. The audio thread activates it via the
 // existing 50 ms crossfade, without depending on a GUI timer.
 const int next=1-activeEqBank.load(std::memory_order_acquire);
 for(int b=0;b<bands;++b){
  const double f=25.0*std::pow(20000.0/25.0,(b+0.5)/(double)bands);
  frequencies[(size_t)b]=f;
  auto coeff=juce::dsp::IIR::Coefficients<float>::makePeakFilter(
   rate,(float)juce::jmin(f,rate*0.45),4.0f,
   juce::Decibels::decibelsToGain(savedMatchGains[(size_t)b]));
  for(int c=0;c<2;++c)
   *eqBanks[(size_t)next][(size_t)c][(size_t)b].coefficients=*coeff;
 }
 for(auto& channel:eqBanks[(size_t)next])
  for(auto& filter:channel)filter.reset();
 pendingEqBank.store(next,std::memory_order_release);
}
void DrumBusProcessor::getStateInformation(juce::MemoryBlock& out){
 auto xml=state.copyState().createXml();
 {
  juce::SpinLock::ScopedLockType lock(eqLock);
  auto* eqState=xml->createNewChildElement("MATCH_EQ_SNAPSHOT");
  eqState->setAttribute("valid",savedMatchValid);
  if(savedMatchValid)
   for(int b=0;b<bands;++b)
    eqState->setAttribute("band"+juce::String(b),(double)savedMatchGains[(size_t)b]);
 }
 copyXmlToBinary(*xml,out);
}
void DrumBusProcessor::setStateInformation(const void* d,int n){
 auto xml=getXmlFromBinary(d,n);
 if(!xml||!xml->hasTagName(state.state.getType()))return;
 state.replaceState(juce::ValueTree::fromXml(*xml));
 {
  juce::SpinLock::ScopedLockType lock(eqLock);
  savedMatchValid=false;
  if(const auto* eqState=xml->getChildByName("MATCH_EQ_SNAPSHOT")){
   if(eqState->getBoolAttribute("valid",false)){
    bool valid=true;
    std::array<float,bands> restored{};
    for(int b=0;b<bands;++b){
     const auto name="band"+juce::String(b);
     if(!eqState->hasAttribute(name)){valid=false;break;}
     const double value=eqState->getDoubleAttribute(name);
     if(!std::isfinite(value)||value<-30.0||value>30.0){valid=false;break;}
     restored[(size_t)b]=(float)value;
    }
    if(valid){savedMatchGains=restored;savedMatchValid=true;}
   }
  }
 }
 if(savedMatchValid)restoreSavedMatch();
 else{
  matched.store(false,std::memory_order_release);
  pendingEqBank.store(-1,std::memory_order_release);
 }
}
juce::AudioProcessorEditor* DrumBusProcessor::createEditor(){return new DrumBusEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DrumBusProcessor();}
