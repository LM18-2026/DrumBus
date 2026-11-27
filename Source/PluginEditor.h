#pragma once
#include "PluginProcessor.h"
class DrumBusEditor : public juce::AudioProcessorEditor, private juce::Timer, public juce::FileDragAndDropTarget {
public:
 explicit DrumBusEditor(DrumBusProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 ~DrumBusEditor() override {stopTimer();refChooser.reset();if(refWorker)refWorker->stopThread(-1);setLookAndFeel(nullptr);}
 bool isInterestedInFileDrag(const juce::StringArray& files) override {return files.size()==1&&juce::File(files[0]).hasFileExtension("wav;aif;aiff;flac");}
 void filesDropped(const juce::StringArray& files,int,int) override {if(isInterestedInFileDrag(files))startReference(juce::File(files[0]));}
private:
 juce::TextButton loadRef{"LOAD REF"};
 juce::Label customName;
 std::unique_ptr<juce::FileChooser> refChooser;
 struct RefWorker : juce::Thread {
  RefWorker():juce::Thread("Reference FFT"){}
  juce::File file;DrumBusProcessor::CustomReference result;std::atomic<bool> done{false};
  void run() override {result=DrumBusProcessor::analyseCustomFile(file);done.store(true);}
 };
 std::unique_ptr<RefWorker> refWorker;
 void startReference(const juce::File&);
 struct NumericSlider : juce::Slider {
  double getValueFromText(const juce::String& text) override { return juce::Slider::getValueFromText(text.replaceCharacter(',', '.')); }
  void mouseDoubleClick(const juce::MouseEvent& e) override {
   if(e.mods.isCtrlDown() && e.eventComponent != this){setValue(resetValue,juce::sendNotificationSync);return;}
   juce::Slider::mouseDoubleClick(e);
  }
  void mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel) override {
   auto w=wheel;w.deltaX*=0.1f;w.deltaY*=0.1f;juce::Slider::mouseWheelMove(e,w);
  }
  void mouseDown(const juce::MouseEvent& e) override {
   if(e.eventComponent!=this)return;
   setMouseDragSensitivity(e.mods.isShiftDown()?2500:250);
   juce::Slider::mouseDown(e);

  }
  void mouseDrag(const juce::MouseEvent& e) override {
   if(e.eventComponent!=this)return;
   setMouseDragSensitivity(e.mods.isShiftDown()?2500:250);juce::Slider::mouseDrag(e);
  }
  double resetValue=0;
 };
 struct SwitchLook : juce::LookAndFeel_V4 {
  juce::Label* createSliderTextBox(juce::Slider& slider) override {
   auto* label=juce::LookAndFeel_V4::createSliderTextBox(slider);
   label->setEditable(true,false,true);
   label->onEditorShow=[label]{if(auto* ed=label->getCurrentTextEditor())ed->selectAll();};
   return label;
  }
  int getSliderThumbRadius(juce::Slider& s) override {return juce::roundToInt(1.25f*juce::LookAndFeel_V4::getSliderThumbRadius(s));}
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
 std::array<juce::Rectangle<int>,6> panels{};
 bool lastEnd=false,lastReverse=false;
 juce::ToggleButton powerEnabled{"ON"},powerDelta{"SAT DELTA"},powerCut{"LOCUT 250 Hz"};
 juce::TextButton rmsMatch{"AUTO GAIN"},pluginReset{"RESET PLUGIN"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerDeltaA,powerCutA;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;
 void timerCallback() override {
  bool e=processor.state.getRawParameterValue("deessEnd")->load()>0.5f, r=processor.state.getRawParameterValue("moduleOrder")->load()>0.5f;
  if(e!=lastEnd||r!=lastReverse){lastEnd=e;lastReverse=r;resized();}
  dsEnd.setButtonText(processor.state.getRawParameterValue("deessEnd")->load()>0.5f?"END OF CHAIN":"POST AUTO EQ");
  moduleOrder.setButtonText(processor.state.getRawParameterValue("moduleOrder")->load()>0.5f?"1176 > AMBIENCE":"AMBIENCE > 1176");
  powerDelta.setButtonText(powerDelta.getToggleState()?"SAT DELTA":"SAT FULL");
  const int ag=processor.autoGainStatus.load();rmsMatch.setButtonText(ag==1||ag==2?"MEASURING":ag==3?"RETRY GAIN":"AUTO GAIN");rmsMatch.setEnabled(ag!=1&&ag!=2);
  if(refWorker&&refWorker->done.load()){
   refWorker->stopThread(-1);
   if(refWorker->result.valid){processor.setCustomReference(refWorker->result);customName.setText(refWorker->result.name,juce::dontSendNotification);}
   else customName.setText("Invalid or silent file",juce::dontSendNotification);
   refWorker.reset();loadRef.setEnabled(true);loadRef.setButtonText("LOAD REF");
  }
  processor.getEqResponse(responseCurve);
  if(++grNumericTicks>=3){grNumericTicks=0;grNumericDisplayed=processor.compReductionDb.load(std::memory_order_relaxed);}
  repaint();
 }
 DrumBusProcessor& processor;
 DrumBusProcessor::EqResponse responseCurve{};
 int grNumericTicks=0;
 float grNumericDisplayed=0.f;
 juce::TextButton analyse{"EQ LISTEN (5 s)"},autoEqReset{"RESET"};
 juce::ToggleButton enabled{"ON"},tiltEnabled{"TILT EQ"};
 NumericSlider matchOutput,globalOutput;
 juce::ToggleButton globalBypass{"BYPASS"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> matchOutputAttachment,globalOutputAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> globalBypassAttachment;
 NumericSlider blend,depth,tiltAmount,reverbWet,reverbLength;
 juce::ToggleButton reverbEnabled{"ON"},compEnabled{"ON"};
 NumericSlider compInput,compOutput,compAttack,compRelease;
 juce::ComboBox compRatio,compMode,referenceType;
 juce::TextButton moduleOrder{"AMBIENCE > 1176"};
 juce::TextButton meterReset{"RESET"};
 juce::ToggleButton sweetEnabled{"ON"};
 NumericSlider sweetBass,sweetTreble,sweetFreq,sweetGain,sweetQ,sweetSat;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sweetEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sweetBassAttachment,sweetTrebleAttachment,
  sweetFreqAttachment,sweetGainAttachment,sweetQAttachment,sweetSatAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment,tiltEnabledAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment,depthAttachment,tiltAmountAttachment,reverbWetAttachment,reverbLengthAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbEnableAttachment,compEnableAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compInputAttachment,compOutputAttachment,compAttackAttachment,compReleaseAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> compRatioAttachment,compModeAttachment,referenceAttachment;
 std::array<std::array<NumericSlider,6>,2> dsSliders;
 std::array<std::array<juce::ToggleButton,3>,2> dsButtons;
 juce::ToggleButton dsDual{"DUAL MONO"},dsEnd{"END OF CHAIN"};
 std::array<std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,6>,2> dsSA;
 std::array<std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,3>,2> dsBA;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dsDualA,dsEndA;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumBusEditor)
};
