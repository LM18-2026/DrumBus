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
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"reverbMix",1},"Reverb Dry Wet",juce::NormalisableRange<float>(0.f,100.f,1.f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"reverbLength",1},"Reverb IR Length",juce::NormalisableRange<float>(-100.f,100.f,1.f),0.f));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reverbEnable",1},"Reverb Enabled",true));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"clipEnable",1},"Clipper Enabled",false));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"clipListen",1},"Clipper Listen",false));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"clipReduction",1},"Clip Reduction dB",juce::NormalisableRange<float>(0.f,12.f,0.1f),0.f));
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
 captureSize=(int)std::ceil(sr*5.0);
 captured.setSize(1,captureSize);
 clipCapturedPre.setSize(2,captureSize);
 clipCapturedOriginal.setSize(2,captureSize);
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
 clipSamples=0;clipObservedPeak=0.f;clipOriginalPeak=0.f;clipWasListening=false;
 clipCaptureComplete.store(false);clipMakeup.store(1.f);clipCalibratedReduction=-1.f;
 clipReady.store(false);clipAnalysisPercent.store(0);
 clipThreshold=1.f;
 clipThresholdSmooth.reset(sr,0.05);
 clipThresholdSmooth.setCurrentAndTargetValue(1.f);
 auto pre=juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr,300.0,0.70710678f,juce::Decibels::decibelsToGain(-24.f));
 auto post=juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr,300.0,0.70710678f,juce::Decibels::decibelsToGain(24.f));
 for(int c=0;c<2;++c){
  *clipPre[(size_t)c].coefficients=*pre;
  *clipPost[(size_t)c].coefficients=*post;
  *clipPostClean[(size_t)c].coefficients=*post;
  clipPre[(size_t)c].reset();clipPost[(size_t)c].reset();clipPostClean[(size_t)c].reset();
 }
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
}
void DrumBusProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals nd;
 const int n=buffer.getNumSamples();
 if(buffer.getNumChannels()<2)return;
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
 // Clipper: analyse the bass-attenuated post-reverb signal for five seconds.
 const bool clipListening=*state.getRawParameterValue("clipListen")>0.5f;
 if(clipListening && !clipWasListening){
  clipReady.store(false,std::memory_order_release);
  clipCaptureComplete.store(false,std::memory_order_release);
  clipMakeup.store(1.f,std::memory_order_release);
  clipSamples=0;clipObservedPeak=0.f;clipOriginalPeak=0.f;clipCalibratedReduction=-1.f;
  clipAnalysisPercent.store(0);
 }
 clipWasListening=clipListening;
 const bool clipping=*state.getRawParameterValue("clipEnable")>0.5f;
 const float reduction=juce::jlimit(0.f,12.f,state.getRawParameterValue("clipReduction")->load());
 const bool active=clipping && reduction>0.f && clipReady.load(std::memory_order_acquire);
 // Keep pre-shelf states warm even while bypassed, to avoid a startup transient.
 for(int i=0;i<n;++i){
  const float a=buffer.getSample(0,i),b=buffer.getSample(1,i);
  const float preL=clipPre[0].processSample(a);
  const float preR=clipPre[1].processSample(b);
  if(clipListening && !clipCaptureComplete.load(std::memory_order_relaxed)){
   // Analyse the bass-attenuated signal actually entering the clipper.
   clipObservedPeak=juce::jmax(clipObservedPeak,std::abs(preL),std::abs(preR));
   clipOriginalPeak=juce::jmax(clipOriginalPeak,std::abs(a),std::abs(b));
   clipCapturedPre.setSample(0,clipSamples,preL);
   clipCapturedPre.setSample(1,clipSamples,preR);
   clipCapturedOriginal.setSample(0,clipSamples,a);
   clipCapturedOriginal.setSample(1,clipSamples,b);
   if(++clipSamples>=captureSize){
    clipThreshold=juce::jmax(1.e-6f,clipObservedPeak);
    clipCaptureComplete.store(true,std::memory_order_release);
    clipAnalysisPercent.store(100);
   }else if((clipSamples&1023)==0)
    clipAnalysisPercent.store((int)(100.0*clipSamples/captureSize));
  }
  const float threshold=juce::jmax(1.e-6f,clipThreshold*juce::Decibels::decibelsToGain(-reduction));
  clipThresholdSmooth.setTargetValue(threshold);
  const float t=clipThresholdSmooth.getNextValue();
  const float limitedL=juce::jlimit(-t,t,preL);
  const float limitedR=juce::jlimit(-t,t,preR);
  const float postL=clipPost[0].processSample(limitedL);
  const float postR=clipPost[1].processSample(limitedR);
  // Keep the parallel inverse shelf updated for smooth state continuity.
  clipPostClean[0].processSample(preL);
  clipPostClean[1].processSample(preR);
  if(active){
   // Clip only the bass-attenuated path. Restore bass with the inverse
   // shelf, then raise the complete output by the selected reduction.
   const float makeup=clipMakeup.load(std::memory_order_relaxed);
   buffer.setSample(0,i,postL*makeup);
   buffer.setSample(1,i,postR*makeup);
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
void DrumBusProcessor::calibrateClipper(){
 if(!clipCaptureComplete.load(std::memory_order_acquire))return;
 const float reduction=juce::jlimit(0.f,12.f,state.getRawParameterValue("clipReduction")->load());
 if(clipReady.load(std::memory_order_acquire) && reduction==clipCalibratedReduction)return;
 // Reconstruct the captured five-second signal using a separate pair of
 // inverse shelves. Never mutate the live audio filters from the timer.
 auto coefficients=juce::dsp::IIR::Coefficients<float>::makeLowShelf(
  rate,300.0,0.70710678f,juce::Decibels::decibelsToGain(24.f));
 const float threshold=juce::jmax(1.e-6f,clipThreshold*juce::Decibels::decibelsToGain(-reduction));
 double reconstructedPeak=0.0;
 for(int c=0;c<2;++c){
  juce::dsp::IIR::Filter<float> inverse;
  *inverse.coefficients=*coefficients;
  inverse.reset();
  const float* samples=clipCapturedPre.getReadPointer(c);
  for(int i=0;i<captureSize;++i){
   const float clipped=juce::jlimit(-threshold,threshold,samples[i]);
   reconstructedPeak=juce::jmax(reconstructedPeak,(double)std::abs(inverse.processSample(clipped)));
  }
 }
 const float original=clipOriginalPeak;
 // Keep the original peak after bass restoration. This is a calibration
 // of the analysed five-second section, not a guarantee for future audio.
 const float makeup=(original>1.e-8f && reconstructedPeak>1.e-8)
  ?juce::jlimit(0.01f,32.f,(float)(original/reconstructedPeak)):1.f;
 clipMakeup.store(makeup,std::memory_order_release);
 clipCalibratedReduction=reduction;
 clipReady.store(true,std::memory_order_release);
}
void DrumBusProcessor::timerCallback(){
 calibrateClipper();
 const float length=*state.getRawParameterValue("reverbLength");
 if(length!=lastReverbLength){loadScaledReverb(length);lastReverbLength=length;}

 if(clipReady.load(std::memory_order_acquire) && *state.getRawParameterValue("clipListen")>0.5f)
  if(auto* p=state.getParameter("clipListen"))p->setValueNotifyingHost(0.f);
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
 if(clipReady.load(std::memory_order_acquire) && *state.getRawParameterValue("clipListen")>0.5f)
  if(auto* p=state.getParameter("clipListen"))p->setValueNotifyingHost(0.f);
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
  const double ref=reference[(size_t)b];
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
void DrumBusProcessor::getStateInformation(juce::MemoryBlock& out){
 auto xml=state.copyState().createXml();copyXmlToBinary(*xml,out);
}
void DrumBusProcessor::setStateInformation(const void* d,int n){
 auto xml=getXmlFromBinary(d,n);
 if(xml&&xml->hasTagName(state.state.getType()))state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* DrumBusProcessor::createEditor(){return new DrumBusEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DrumBusProcessor();}
