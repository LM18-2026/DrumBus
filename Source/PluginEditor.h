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
 juce::ToggleButton enabled{"MATCH EQ"};
 juce::Slider blend,depth,reverbWet,reverbLength;
 juce::ToggleButton reverbEnabled{"REVERB ON"},compEnabled{"COMPRESSOR ON"};
 juce::Slider compInput,compOutput,compAttack,compRelease;
 juce::ComboBox compRatio;
 juce::TextButton meterReset{"RESET"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment,depthAttachment,reverbWetAttachment,reverbLengthAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbEnableAttachment,compEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compInputAttachment,compOutputAttachment,compAttackAttachment,compReleaseAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> compRatioAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
