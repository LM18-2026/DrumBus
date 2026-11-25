#pragma once
#include "PluginProcessor.h"
class DrumBusEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit DrumBusEditor(DrumBusProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
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
 juce::Slider matchOutput,globalOutput;
 juce::ToggleButton globalBypass{"BYPASS"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> matchOutputAttachment,globalOutputAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> globalBypassAttachment;
 juce::Slider blend,depth,tiltAmount,reverbWet,reverbLength;
 juce::ToggleButton reverbEnabled{"REVERB ON"},compEnabled{"COMPRESSOR ON"};
 juce::Slider compInput,compOutput,compAttack,compRelease;
 juce::ComboBox compRatio,compMode,referenceType,moduleOrder;
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
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> compRatioAttachment,compModeAttachment,referenceAttachment,moduleOrderAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
