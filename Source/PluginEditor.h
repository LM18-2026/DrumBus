#pragma once
#include "PluginProcessor.h"
class DrumBusEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit DrumBusEditor(DrumBusProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 // Match existing dark/cyan DrumBus styling without changing the controls.
 struct ModuleButtonLookAndFeel final : juce::LookAndFeel_V4 {
  void drawToggleButton(juce::Graphics& g,juce::ToggleButton& button,
                        bool highlighted,bool pressed) override {
   const auto bounds=button.getLocalBounds().toFloat().reduced(1.5f);
   const bool on=button.getToggleState();
   const auto accent=juce::Colour(0xff3dbadf);
   const auto outline=on?accent:juce::Colour(0xff667c90);
   const auto fill=on?juce::Colour(0xff174457):juce::Colour(0xff1a2532);
   g.setColour(fill.brighter(pressed?0.15f:highlighted?0.08f:0.f));
   g.fillRoundedRectangle(bounds,5.f);
   g.setColour(outline);
   g.drawRoundedRectangle(bounds,5.f,1.5f);
   g.setColour(on?juce::Colours::white:juce::Colour(0xffa8b8ca));
   g.setFont(juce::Font(juce::FontOptions(13.f).withStyle("Bold")));
   g.drawFittedText(button.getButtonText(),button.getLocalBounds().reduced(7,2),
                    juce::Justification::centred,1);
  }
 } moduleButtonLookAndFeel;
 void timerCallback() override {
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
 juce::Slider blend,depth,tiltAmount,reverbWet,reverbLength;
 juce::ToggleButton reverbEnabled{"REVERB ON"},compEnabled{"COMPRESSOR ON"};
 juce::Slider compInput,compOutput,compAttack,compRelease;
 juce::ComboBox compRatio,compMode;
 juce::TextButton meterReset{"RESET"};
 juce::ToggleButton sweetEnabled{"SWEETENING ON"};
 juce::Slider sweetBass,sweetTreble,sweetFreq,sweetGain,sweetQ,sweetSat;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sweetEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sweetBassAttachment,sweetTrebleAttachment,
  sweetFreqAttachment,sweetGainAttachment,sweetQAttachment,sweetSatAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment,tiltEnabledAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment,depthAttachment,tiltAmountAttachment,reverbWetAttachment,reverbLengthAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbEnableAttachment,compEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compInputAttachment,compOutputAttachment,compAttackAttachment,compReleaseAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> compRatioAttachment,compModeAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
