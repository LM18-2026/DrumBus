#pragma once
#include "PluginProcessor.h"
class DrumBusEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit DrumBusEditor(DrumBusProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 void timerCallback() override {repaint();}
 DrumBusProcessor& processor;
 juce::TextButton analyse{"EQ LISTEN (5 s)"};
 juce::ToggleButton enabled{"MATCH EQ"};
 juce::Slider blend,depth,reverbWet,reverbLength;
 juce::ToggleButton reverbEnabled{"REVERB ON"};
 juce::ComboBox phase;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment,depthAttachment,reverbWetAttachment,reverbLengthAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbEnableAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
