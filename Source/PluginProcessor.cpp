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
 for(int i=1;i<fftSize/2;++i){
  int b=bandForFrequency(i*rate/fftSize);
  if(b>=0){
   const double re=fftData[2*i],im=fftData[2*i+1];
   totals[(size_t)b]+=(re*re+im*im);
  }
 }
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
    for(int i=0;i<n;++i)data[i]=(1.f-blend)*data[i]+blend*wet[i];
   }
  }
 }
}
void DrumBusProcessor::timerCallback(){
 if(!readyToMatch.exchange(false))return;
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
 // Compare normalized spectral shapes, not the absolute recording levels.
 double refTotal=0.0,inTotal=0.0;
 for(int b=0;b<bands;++b){refTotal+=reference[b];inTotal+=input[b]/frames;}
 if(refTotal<1.e-12 || inTotal<1.e-12){matched.store(false);return;}
 refTotal=juce::jmax(refTotal,1.e-18);
 inTotal=juce::jmax(inTotal,1.e-18);
 std::array<float,bands> gains{};
 float positiveSum=0.f;
 for(int b=0;b<bands;++b){
  const double ref=reference[b]/refTotal,actual=(input[b]/frames)/inTotal;
  // Stronger individual correction, but prevent the sum of positive boosts
  // from exceeding 6 dB even when the overlapping filters interact.
  gains[(size_t)b]=juce::jlimit(-6.f,6.f,(float)(10.0*std::log10((ref+1e-10)/(actual+1e-10))));
  positiveSum+=juce::jmax(0.f,gains[(size_t)b]);
 }
 const float positiveScale=positiveSum>6.f?6.f/positiveSum:1.f;
 juce::SpinLock::ScopedLockType lock(eqLock);
 for(int b=0;b<bands;++b){
  const double f=25.0*std::pow(20000.0/25.0,(b+0.5)/(double)bands);
  frequencies[b]=f;
  const float gain=gains[(size_t)b]>0.f?gains[(size_t)b]*positiveScale:gains[(size_t)b];
  const float center=(float)juce::jmin(f,rate*0.45);
  auto coeff=juce::dsp::IIR::Coefficients<float>::makePeakFilter(rate,center,2.0f,juce::Decibels::decibelsToGain(gain));
  for(int c=0;c<2;++c)*eq[(size_t)c][(size_t)b].coefficients=*coeff;
 }
 for(auto& channel:eq)for(auto& filter:channel)filter.reset();
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
