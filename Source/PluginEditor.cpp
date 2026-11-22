#include "PluginEditor.h"
DrumBusEditor::DrumBusEditor(DrumBusProcessor& p):juce::AudioProcessorEditor(&p),processor(p){
 setSize(1200,620);
 addAndMakeVisible(analyse);addAndMakeVisible(enabled);addAndMakeVisible(phase);addAndMakeVisible(blend);addAndMakeVisible(depth);addAndMakeVisible(reverbWet);addAndMakeVisible(reverbLength);addAndMakeVisible(reverbEnabled);addAndMakeVisible(clipEnabled);addAndMakeVisible(clipListen);addAndMakeVisible(clipReduction);
 // LinearBar is an editable numeric field with host automation support.
 clipReduction.setSliderStyle(juce::Slider::LinearBar);
 clipReduction.setTextBoxStyle(juce::Slider::TextBoxLeft,false,90,30);
 clipReduction.setNumDecimalPlacesToDisplay(1);
 clipReductionAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"clipReduction",clipReduction);
 clipEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"clipEnable",clipEnabled);
 clipListen.onClick=[this]{if(auto* p=processor.state.getParameter("clipListen")){p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();}};
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
 reverbWetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"reverbMix",reverbWet);
 reverbLengthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"reverbLength",reverbLength);
 reverbEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"reverbEnable",reverbEnabled);
 analyse.onClick=[this]{
  if(auto* p=processor.state.getParameter("listen")){
   p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();
  }
 };
 phase.addItem("MinPhase",1);phase.addItem("LinPhase",2);

 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"enable",enabled);
 phaseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"phase",phase);
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
 g.drawText("V0.18  |  GUI",1030,20,140,30,juce::Justification::centredRight);
 auto section=[&](juce::Rectangle<int> r,const juce::String& title){
  g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),12.f);
  g.setColour(line);g.drawRoundedRectangle(r.toFloat().reduced(1.f),12.f,1.5f);
  g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(20.f).withStyle("Bold")));
  g.drawText(title,r.getX()+18,r.getY()+15,r.getWidth()-36,35,juce::Justification::left);
 };
 section({18,80,570,455},"MATCH EQ");
 section({600,80,260,455},"REVERB / AMBIENCE");
 section({872,80,310,455},"HARD CLIPPER");
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
 g.drawText("5 s PEAK ANALYSIS",892,145,270,30,juce::Justification::centred);
 g.drawText("SHELF  ->  CLIP  ->  INV. SHELF",887,235,280,50,juce::Justification::centred);
 g.drawText("CLIP REDUCTION (dB)",887,325,280,35,juce::Justification::centred);
 g.drawText(processor.clipReady.load()?"CLIP READY":("ANALYSIS  "+juce::String(processor.clipAnalysisPercent.load())+" %"),887,410,280,35,juce::Justification::centred);
 g.setColour(juce::Colour(0xffa8b8ca));g.drawText("IN / OUT METERS  -  planned",20,557,1160,40,juce::Justification::centred);
}
void DrumBusEditor::resized(){
 analyse.setBounds(38,155,145,40);
 blend.setBounds(155,492,390,35);
 depth.setBounds(170,203,375,35);
 enabled.setBounds(440,455,125,30);
 phase.setBounds(335,155,225,40);
 reverbWet.setBounds(612,210,116,125);
 reverbLength.setBounds(734,210,116,125);
 reverbEnabled.setBounds(668,455,125,30);
 clipListen.setBounds(900,185,245,38);
 clipReduction.setBounds(975,365,100,38);
 clipEnabled.setBounds(964,455,125,30);
}
