#include "PluginEditor.h"
DrumBusEditor::DrumBusEditor(DrumBusProcessor& p):juce::AudioProcessorEditor(&p),processor(p){
 setSize(1200,620);
 addAndMakeVisible(analyse);addAndMakeVisible(enabled);addAndMakeVisible(phase);addAndMakeVisible(blend);
 blend.setSliderStyle(juce::Slider::LinearHorizontal);
 blend.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
 blend.setNumDecimalPlacesToDisplay(0);
 blendAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"blend",blend);
 analyse.onClick=[this]{
  if(auto* p=processor.state.getParameter("listen")){
   p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();
  }
 };
 phase.addItem("MinPhase",1);phase.addItem("LinPhase",2);

 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"enable",enabled);
 phaseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"phase",phase);
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
 g.drawText("V0.6  |  GUI",1030,20,140,30,juce::Justification::centredRight);
 auto section=[&](juce::Rectangle<int> r,const juce::String& title){
  g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),12.f);
  g.setColour(line);g.drawRoundedRectangle(r.toFloat().reduced(1.f),12.f,1.5f);
  g.setColour(juce::Colours::white);g.setFont(juce::Font(juce::FontOptions(20.f).withStyle("Bold")));
  g.drawText(title,r.getX()+18,r.getY()+15,r.getWidth()-36,35,juce::Justification::left);
 };
 section({18,80,570,455},"MATCH EQ");
 section({600,80,260,455},"REVERB (IR)");
 section({872,80,310,455},"HARD CLIPPER");
 const juce::Rectangle<float> graph(48.f,245.f,510.f,205.f);
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
 g.drawText("30-band response display: not yet implemented",60,310,485,40,juce::Justification::centred);
 const juce::String status=processor.matched.load()?"MATCH READY":("ANALYSIS  "+juce::String(processor.analysisPercent.load())+" %");
 g.setColour(line);g.drawText(status,42,456,515,25,juce::Justification::centred);
 g.setColour(juce::Colours::white);g.drawText("EQ BLEND",40,495,110,25,juce::Justification::centredLeft);
 g.setColour(juce::Colour(0xffa8b8ca));
 g.drawText("DRY / WET",620,145,220,30,juce::Justification::centred);
 g.drawText("IR LENGTH  -100 ... +100",610,245,240,30,juce::Justification::centred);
 g.drawText("Reverb DSP not implemented",610,355,240,40,juce::Justification::centred);
 g.drawText("5 s ANALYSIS",892,145,270,30,juce::Justification::centred);
 g.drawText("SHELF  ->  CLIP  ->  INV. SHELF",887,235,280,50,juce::Justification::centred);
 g.drawText("CLIP REDUCTION  0 ... 6 dB",887,325,280,35,juce::Justification::centred);
 g.drawText("Clipper DSP not implemented",887,395,280,35,juce::Justification::centred);
 g.setColour(juce::Colour(0xffa8b8ca));g.drawText("IN / OUT METERS  -  planned",20,557,1160,40,juce::Justification::centred);
}
void DrumBusEditor::resized(){
 analyse.setBounds(38,155,145,40);
 blend.setBounds(155,492,390,35);
 enabled.setBounds(195,155,125,40);
 phase.setBounds(335,155,225,40);
}
