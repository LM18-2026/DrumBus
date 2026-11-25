#pragma once
#include "PluginProcessor.h"
class DrumBusEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit DrumBusEditor(DrumBusProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 ~DrumBusEditor() override { setLookAndFeel(nullptr); }
private:
 struct NumericSlider : juce::Slider {
  double getValueFromText(const juce::String& text) override { return juce::Slider::getValueFromText(text.replaceCharacter(',', '.')); }
  void mouseDoubleClick(const juce::MouseEvent& e) override {
   if(e.mods.isCtrlDown() && e.eventComponent != this){setValue(resetValue,juce::sendNotificationSync);return;}
   juce::Slider::mouseDoubleClick(e);
  }
  double resetValue=0;
 };
 struct SwitchLook : juce::LookAndFeel_V4 {
  void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool,bool) override {
   auto r=b.getLocalBounds().toFloat().reduced(1.f);
   auto cyan=juce::Colour(0xff3dbadf);
   g.setColour(cyan);
   if(b.getToggleState())g.fillRoundedRectangle(r,4.f);
   else g.drawRoundedRectangle(r,4.f,1.f);
   g.setColour(b.getToggleState()?juce::Colour(0xff101721):juce::Colours::white);
   g.setFont(juce::Font(juce::FontOptions(13.f)));
   g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(4),juce::Justification::centred,1);
  }
 } switchLook;
 void timerCallback() override {
  moduleOrder.setButtonText(processor.state.getRawParameterValue("moduleOrder")->load()>0.5f?"1176 > AMBIENCE":"AMBIENCE > 1176");
  processor.getEqResponse(responseCurve);
  if(++grNumericTicks>=3){grNumericTicks=0;grNumericDisplayed=processor.compReductionDb.load(std::memory_order_relaxed);}
  repaint();
 }
 DrumBusProcessor& processor;
 DrumBusProcessor::EqResponse responseCurve{};
 int grNumericTicks=0;
 float grNumericDisplayed=0.f;
 juce::TextButton analyse{"EQ LISTEN (5 s)"};
 juce::ToggleButton enabled{"MATCH EQ"},tiltEnabled{"TILT EQ"};
 NumericSlider matchOutput,globalOutput;
 juce::ToggleButton globalBypass{"BYPASS"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> matchOutputAttachment,globalOutputAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> globalBypassAttachment;
 NumericSlider blend,depth,tiltAmount,reverbWet,reverbLength;
 juce::ToggleButton reverbEnabled{"REVERB ON"},compEnabled{"COMPRESSOR ON"};
 NumericSlider compInput,compOutput,compAttack,compRelease;
 juce::ComboBox compRatio,compMode,referenceType;
 juce::TextButton moduleOrder{"AMBIENCE > 1176"};
 juce::TextButton meterReset{"RESET"};
 juce::ToggleButton sweetEnabled{"SWEETENING ON"};
 NumericSlider sweetBass,sweetTreble,sweetFreq,sweetGain,sweetQ,sweetSat;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sweetEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sweetBassAttachment,sweetTrebleAttachment,
  sweetFreqAttachment,sweetGainAttachment,sweetQAttachment,sweetSatAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment,tiltEnabledAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment,depthAttachment,tiltAmountAttachment,reverbWetAttachment,reverbLengthAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbEnableAttachment,compEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compInputAttachment,compOutputAttachment,compAttackAttachment,compReleaseAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> compRatioAttachment,compModeAttachment,referenceAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
