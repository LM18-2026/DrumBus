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
 juce::ToggleButton analyse{"LISTEN 5 s"},enabled{"MATCH EQ"};
 juce::ComboBox phase;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttachment,enableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> phaseAttachment;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
