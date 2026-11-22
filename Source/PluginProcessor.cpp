#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>
DrumBusProcessor::DrumBusProcessor()
 :juce::AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 state(*this,nullptr,"PARAMS",params()){
 for(int k=0;k<fftSize;++k)window[k]=0.5f-0.5f*std::cos(juce::MathConstants<float>::twoPi*(float)k/(fftSize-1));
 startTimerHz(10);
}
juce::AudioProcessorValueTreeState::ParameterLayout DrumBusProcessor::params(){
 std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"listen",1},"Analyse",false));
 p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"phase",1},"Phase",juce::StringArray{"MinPhase","LinPhase"},0));
 p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"enable",1},"Match EQ",true));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"blend",1},"EQ Blend",juce::NormalisableRange<float>(0.f,100.f,1.f),100.f));
 p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"strength",1},"Match Depth",juce::NormalisableRange<float>(0.f,100.f,1.f),35.f));
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
 processed.setSize(2,juce::jmax(1,block));
 captured.clear();
 capturedCount.store(0);
 readyToMatch.store(false);
 matched.store(false);
 analysisPercent.store(0);
 wasListening=false;
 lastStrength=*state.getRawParameterValue("strength");
 for(auto& channel:eq)for(auto& filter:channel)filter.reset();
 analyseReference();
}
void DrumBusProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals nd;
 const int n=buffer.getNumSamples();
 if(buffer.getNumChannels()<2)return;
 const bool listening=*state.getRawParameterValue("listen")>0.5f;
 if(listening && !wasListening){
  // A new session always discards the previous EQ before recording.
  matched.store(false,std::memory_order_release);
  capturedCount.store(0,std::memory_order_release);
  analysisPercent.store(0);
  readyToMatch.store(false,std::memory_order_release);
 }
 wasListening=listening;
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
 if(blend>0.f && *state.getRawParameterValue("enable")>0.5f && matched.load(std::memory_order_acquire)){
  juce::SpinLock::ScopedTryLockType lock(eqLock);
  if(lock.isLocked()){
   // Process into preallocated scratch buffer. The EQ gain budget is enforced at match creation.
   if(n>processed.getNumSamples())return;
   for(int c=0;c<2;++c){
    auto* data=buffer.getReadPointer(c);
    auto* dest=processed.getWritePointer(c);
    for(int i=0;i<n;++i){
     float v=data[i];
     for(auto& filter:eq[(size_t)c])v=filter.processSample(v);
     dest[i]=v;

    }
   }
   for(int c=0;c<2;++c){
    auto* data=buffer.getWritePointer(c);
    const auto* wet=processed.getReadPointer(c);
    for(int i=0;i<n;++i)data[i]=(1.f-blend)*data[i]+blend*wet[i]*eqOutputGain;
   }
  }
 }
}
void DrumBusProcessor::timerCallback(){
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
 juce::SpinLock::ScopedLockType lock(eqLock);
 for(int b=0;b<bands;++b){
  const double f=25.0*std::pow(20000.0/25.0,(b+0.5)/(double)bands);
  frequencies[b]=f;
  const float gain=gains[(size_t)b];
  const float center=(float)juce::jmin(f,rate*0.45);
  auto coeff=juce::dsp::IIR::Coefficients<float>::makePeakFilter(rate,center,4.0f,juce::Decibels::decibelsToGain(gain));
  for(int c=0;c<2;++c)*eq[(size_t)c][(size_t)b].coefficients=*coeff;
 }
 for(auto& channel:eq)for(auto& filter:channel)filter.reset();
 eqOutputGain=newOutputGain;
 matched.store(true,std::memory_order_release);
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
