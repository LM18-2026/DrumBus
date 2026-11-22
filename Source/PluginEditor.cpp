#include "PluginEditor.h"
DrumBusEditor::DrumBusEditor(DrumBusProcessor& p):juce::AudioProcessorEditor(&p),processor(p){
 setSize(500,270);
 addAndMakeVisible(analyse);addAndMakeVisible(enabled);addAndMakeVisible(phase);
 phase.addItem("MinPhase",1);phase.addItem("LinPhase",2);
 listenAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"listen",analyse);
 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"enable",enabled);
 phaseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"phase",phase);
 startTimerHz(10);
}
void DrumBusEditor::paint(juce::Graphics& g){
 g.fillAll(juce::Colour(0xff121923));
 g.setColour(juce::Colours::white);
 g.setFont(juce::Font(juce::FontOptions(27.f).withStyle("Bold")));
 g.drawText("LM18  DRUMBUS",20,12,460,48,juce::Justification::centred);
 g.setFont(juce::Font(juce::FontOptions(15.f)));
 g.drawText("MATCH EQ  |  30 BANDS  |  REFNoise1.wav",20,65,460,26,juce::Justification::centred);
 juce::String status=processor.matched.load()?"MATCH READY":("ANALYSIS  "+juce::String(processor.analysisPercent.load())+" %");
 g.drawText(status,20,190,460,30,juce::Justification::centred);
 g.setFont(juce::Font(juce::FontOptions(13.f)));
 g.drawText("LM18  |  V0.1  |  STAGE 1",20,238,460,22,juce::Justification::centred);
}
void DrumBusEditor::resized(){
 analyse.setBounds(30,110,140,40);
 enabled.setBounds(190,110,125,40);
 phase.setBounds(335,110,135,40);
}
