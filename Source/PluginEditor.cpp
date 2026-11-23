#include "PluginEditor.h"
DrumBusEditor::DrumBusEditor(DrumBusProcessor& p):juce::AudioProcessorEditor(&p),processor(p){
 setSize(1200,620);
 addAndMakeVisible(meterReset);
 meterReset.onClick=[this]{processor.resetMeters();};
 addAndMakeVisible(analyse);addAndMakeVisible(enabled);addAndMakeVisible(blend);addAndMakeVisible(depth);addAndMakeVisible(tiltAmount);addAndMakeVisible(tiltEnabled);addAndMakeVisible(reverbWet);addAndMakeVisible(reverbLength);addAndMakeVisible(reverbEnabled);addAndMakeVisible(compEnabled);addAndMakeVisible(compInput);addAndMakeVisible(compOutput);addAndMakeVisible(compAttack);addAndMakeVisible(compRelease);addAndMakeVisible(compRatio);addAndMakeVisible(compMode);
 compEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"compEnable",compEnabled);
 compMode.addItem("STEREO",1);compMode.addItem("DUAL MONO",2);
 compModeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"compMode",compMode);
 auto setup=[&](juce::Slider& slider,const juce::String& id){
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,66,23);
  slider.setNumDecimalPlacesToDisplay(2);
 };
 for(auto* slider:{&compInput,&compOutput,&compAttack,&compRelease})setup(*slider,"");
 compInputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compInput",compInput);
 compOutputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compOutput",compOutput);
 compAttackAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compAttack",compAttack);
 compReleaseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compRelease",compRelease);
 compRatio.addItem("4:1",1);compRatio.addItem("8:1",2);compRatio.addItem("12:1",3);compRatio.addItem("20:1",4);
 compRatioAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"compRatio",compRatio);
 for(auto* knob:{&reverbWet,&reverbLength}){
  knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  knob->setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,24);
  knob->setNumDecimalPlacesToDisplay(0);
 }
 blend.setSliderStyle(juce::Slider::LinearHorizontal);
 blend.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
 blend.setNumDecimalPlacesToDisplay(0);
 blendAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"blend",blend);
 depth.setSliderStyle(juce::Slider::LinearHorizontal);
 depth.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
 depth.setNumDecimalPlacesToDisplay(0);
 depthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"strength",depth);
 tiltAmount.setSliderStyle(juce::Slider::LinearHorizontal);
 tiltAmount.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
 tiltAmount.setNumDecimalPlacesToDisplay(1);
 tiltAmountAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"tiltDb",tiltAmount);
 tiltEnabledAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"tiltEnable",tiltEnabled);
 reverbWetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"reverbMix",reverbWet);
 reverbLengthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"reverbLength",reverbLength);
 reverbEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"reverbEnable",reverbEnabled);
 analyse.onClick=[this]{
  if(auto* p=processor.state.getParameter("listen")){
   p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();
  }
 };

 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"enable",enabled);
 processor.getEqResponse(responseCurve);
 startTimerHz(10);
}
void DrumBusEditor::paint(juce::Graphics& g){
 const auto bg=juce::Colour(0xff101721),panel=juce::Colour(0xff1a2532),line=juce::Colour(0xff3dbadf);
 g.fillAll(bg);
 g.setColour(juce::Colours::white);
 g.setFont(juce::Font(juce::FontOptions(30.f).withStyle("Bold")));
 g.drawText("LM18 DrumBus",25,15,480,45,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xffa8b8ca));
 g.setFont(juce::Font(juce::FontOptions(16.f)));
 g.drawText("V0.27  |  GUI",1030,20,140,30,juce::Justification::centredRight);
 auto section=[&](juce::Rectangle<int> r,const juce::String& title){
  g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),12.f);
  g.setColour(line);g.drawRoundedRectangle(r.toFloat().reduced(1.f),12.f,1.5f);
  g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(20.f).withStyle("Bold")));
  g.drawText(title,r.getX()+18,r.getY()+15,r.getWidth()-36,35,juce::Justification::left);
 };
 section({18,80,570,455},"MATCH EQ");
 section({600,80,260,455},"REVERB / AMBIENCE");
 section({872,80,310,455},"FET COMPRESSOR");
 const juce::Rectangle<float> graph(48.f,245.f,510.f,180.f);
 g.setColour(juce::Colour(0xff0b131d));g.fillRect(graph);
 g.setColour(juce::Colour(0xff33485a));
 for(int i=0;i<=6;++i){
  float y=graph.getY()+i*graph.getHeight()/6.f;
  g.drawHorizontalLine((int)y,graph.getX(),graph.getRight());
 }
 for(int i=0;i<=10;++i){
  float x=graph.getX()+i*graph.getWidth()/10.f;
  g.drawVerticalLine((int)x,graph.getY(),graph.getBottom());
 }
 g.setColour(juce::Colour(0xffa8b8ca));g.setFont(juce::Font(juce::FontOptions(14.f)));
 // Calculated frequency response of the actual cascaded EQ filters.
 // Display spans 20 Hz to 20 kHz and -30 to +30 dB.
 g.setColour(juce::Colour(0xff53677a));
 g.drawHorizontalLine((int)graph.getCentreY(),graph.getX(),graph.getRight());
 juce::Path curve;
 for(size_t i=0;i<responseCurve.size();++i){
  const float x=graph.getX()+graph.getWidth()*(float)i/(float)(responseCurve.size()-1);
  const float db=juce::jlimit(-30.f,30.f,responseCurve[i]);
  const float y=graph.getCentreY()-db*graph.getHeight()/60.f;
  if(i==0)curve.startNewSubPath(x,y);else curve.lineTo(x,y);
 }
 g.setColour(line);
 g.strokePath(curve,juce::PathStrokeType(2.f));
 g.setColour(juce::Colour(0xffa8b8ca));
 g.drawText("+30 dB",50,247,75,18,juce::Justification::left);
 g.drawText("0 dB",50,335,75,18,juce::Justification::left);
 g.drawText("-30 dB",50,404,75,18,juce::Justification::left);
 g.drawText("20 Hz",48,430,65,18,juce::Justification::left);
 g.drawText("1 kHz",293,430,65,18,juce::Justification::centred);
 g.drawText("20 kHz",490,430,68,18,juce::Justification::right);
 const juce::String status=processor.matched.load()?"MATCH READY":("ANALYSIS  "+juce::String(processor.analysisPercent.load())+" %");
 g.setColour(juce::Colours::white);g.drawText("MATCH DEPTH",40,208,125,25,juce::Justification::centredLeft);
 g.setColour(line);g.drawText(status,42,455,300,25,juce::Justification::centred);
 g.setColour(juce::Colours::white);g.drawText("EQ BLEND",40,495,110,25,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xffa8b8ca));
 g.drawText("REVERB BLEND",612,172,116,25,juce::Justification::centred);
 g.drawText("LENGTH",734,172,116,25,juce::Justification::centred);
 g.drawText("-100 ... +100",734,335,116,22,juce::Justification::centred);
 g.drawText("Room: LexRoom1.wav",610,407,240,32,juce::Justification::centred);
 g.drawText("1176-STYLE",888,137,120,25,juce::Justification::centredLeft);
 g.drawText("INPUT",888,180,78,25,juce::Justification::centredLeft);
 g.drawText("OUTPUT",888,225,78,25,juce::Justification::centredLeft);
 g.drawText("ATTACK",888,270,78,25,juce::Justification::centredLeft);
 g.drawText("RELEASE",888,315,78,25,juce::Justification::centredLeft);
 g.drawText("RATIO",888,363,78,25,juce::Justification::centredLeft);
 // Scaled gain-reduction meter, VU-style 300/500 ms ballistics.
 const float gr=juce::jlimit(0.f,30.f,processor.compGrVuDb.load());
 const juce::Rectangle<float> grTrack(903.f,417.f,246.f,12.f);
 g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(13.f).withStyle("Bold")));
 g.drawText("GR",892,395,42,20,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xffa8b8ca));g.setFont(juce::Font(juce::FontOptions(12.f)));
 g.drawText(juce::String(grNumericDisplayed,1)+" dB",1080,395,72,20,juce::Justification::centredRight);
 g.setColour(juce::Colour(0xff243343));g.fillRoundedRectangle(grTrack,3.f);
 if(gr>0.01f){
  g.setColour(juce::Colour(0xff3dbadf));
  g.fillRoundedRectangle(grTrack.withWidth(grTrack.getWidth()*gr/30.f),3.f);
 }
 g.setColour(juce::Colour(0xffa8b8ca));g.setFont(juce::Font(juce::FontOptions(11.f)));
 for(const int tick:{0,5,10,15,20,25,30}){
  const float x=grTrack.getX()+grTrack.getWidth()*(float)tick/30.f;
  g.drawVerticalLine((int)std::round(x),430.f,435.f);
  g.drawText(juce::String(tick),(int)x-12,435,24,15,juce::Justification::centred);
 }
 // VU ballistics: the bar uses a 300 ms power integrator.
 // Peak and RMS text are accumulated from RESET until the next reset.
 auto meter=[&](int x,const juce::String& name,float peak,float rms,float vu){
  const int y=558,w=405,h=14;
  g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(14.f).withStyle("Bold")));
  g.drawText(name,x,y-3,42,24,juce::Justification::centredLeft);
  const int bx=x+45;
  g.setColour(juce::Colour(0xff243343));g.fillRoundedRectangle((float)bx,(float)y,(float)w,(float)h,3.f);
  const float level=juce::jlimit(0.f,1.f,(vu+60.f)/60.f);
  g.setColour(juce::Colour(0xff3dbadf));
  if(level>0.f)g.fillRoundedRectangle((float)bx,(float)y,(float)w*level,(float)h,3.f);
  g.setColour(juce::Colour(0xffa8b8ca));g.setFont(juce::Font(juce::FontOptions(12.f)));
  g.drawText("RMS AVG "+juce::String(rms,1)+" dBFS   PEAK MAX "+juce::String(peak,1)+" dBFS",bx,y+19,w,20,juce::Justification::centred);
 };
 meter(24,"IN",processor.inputPeakDb.load(),processor.inputRmsDb.load(),processor.inputVuDb.load());
 meter(540,"OUT",processor.outputPeakDb.load(),processor.outputRmsDb.load(),processor.outputVuDb.load());
}
void DrumBusEditor::resized(){
 meterReset.setBounds(1075,556,95,30);
 analyse.setBounds(38,155,145,40);
 blend.setBounds(155,492,390,35);
 depth.setBounds(170,203,375,35);
 tiltAmount.setBounds(170,259,375,35);
 tiltEnabled.setBounds(38,259,125,35);
 enabled.setBounds(440,455,125,30);
 reverbWet.setBounds(612,210,116,125);
 reverbLength.setBounds(734,210,116,125);
 reverbEnabled.setBounds(668,455,125,30);
 compInput.setBounds(965,176,195,34);
 compOutput.setBounds(965,221,195,34);
 compAttack.setBounds(965,266,195,34);
 compRelease.setBounds(965,311,195,34);
 compRatio.setBounds(965,357,195,31);
 compMode.setBounds(1012,135,148,29);
 compEnabled.setBounds(962,455,190,30);
}
