#include "PluginEditor.h"
DrumBusEditor::DrumBusEditor(DrumBusProcessor& p):juce::AudioProcessorEditor(&p),processor(p){
 setLookAndFeel(&switchLook);
 
 for(auto* slider:{&matchOutput,&globalOutput}){
  addAndMakeVisible(*slider);
  slider->setSliderStyle(juce::Slider::LinearHorizontal);
  slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
  slider->setNumDecimalPlacesToDisplay(1);
  slider->setTextValueSuffix(" dB");
 }
 addAndMakeVisible(globalBypass);
 matchOutputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"matchOutput",matchOutput);
 globalOutputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"globalOutput",globalOutput);
 globalBypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"globalBypass",globalBypass);
 addAndMakeVisible(moduleOrder);
 moduleOrder.onClick=[this]{if(auto* p=processor.state.getParameter("moduleOrder")){p->beginChangeGesture();p->setValueNotifyingHost(p->getValue()>0.5f?0.f:1.f);p->endChangeGesture();}};
 addAndMakeVisible(referenceType);
 referenceType.addItem("KIT",1);referenceType.addItem("KICK",2);referenceType.addItem("SNARE",3);referenceType.addItem("HIHAT",4);referenceType.addItem("TOMS",5);referenceType.addItem("AG1",6);
 referenceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"referenceType",referenceType);
 addAndMakeVisible(sweetEnabled);
 sweetEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"sweetEnable",sweetEnabled);
 auto addSweet=[&](juce::Slider& slider,const char* id,int digits){
  addAndMakeVisible(slider);
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,70,23);
  slider.setNumDecimalPlacesToDisplay(digits);
 };
 addSweet(sweetBass,"sweetBass",1);addSweet(sweetTreble,"sweetTreble",1);
 addSweet(sweetFreq,"sweetFreq",0);addSweet(sweetGain,"sweetGain",1);
 addSweet(sweetQ,"sweetQ",2);addSweet(sweetSat,"sweetSat",1);
 sweetBassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"sweetBass",sweetBass);
 sweetTrebleAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"sweetTreble",sweetTreble);
 sweetFreqAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"sweetFreq",sweetFreq);
 sweetGainAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"sweetGain",sweetGain);
 sweetQAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"sweetQ",sweetQ);
 sweetSatAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"sweetSat",sweetSat);
 addAndMakeVisible(meterReset);
 meterReset.onClick=[this]{processor.resetMeters();};
 addAndMakeVisible(analyse);addAndMakeVisible(enabled);addAndMakeVisible(blend);addAndMakeVisible(depth);addAndMakeVisible(tiltAmount);addAndMakeVisible(reverbWet);addAndMakeVisible(reverbLength);addAndMakeVisible(reverbEnabled);addAndMakeVisible(compEnabled);addAndMakeVisible(compInput);addAndMakeVisible(compOutput);addAndMakeVisible(compAttack);addAndMakeVisible(compRelease);addAndMakeVisible(compRatio);addAndMakeVisible(compMode);
 compEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"compEnable",compEnabled);
 compMode.addItem("STEREO",1);compMode.addItem("DUAL MONO",2);
 compModeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"compMode",compMode);
 auto setup=[&](juce::Slider& slider,const juce::String& id){
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,66,23);
  slider.setNumDecimalPlacesToDisplay(2);
 };
 for(auto* slider:{&compInput,&compOutput,&compAttack,&compRelease})setup(*slider,"");
 compInputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compInput",compInput);
 compOutputAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compOutput",compOutput);
 compAttackAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compAttack",compAttack);
 compReleaseAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"compRelease",compRelease);
 compRatio.addItem("4:1",1);compRatio.addItem("8:1",2);compRatio.addItem("12:1",3);compRatio.addItem("20:1",4);
 compRatioAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"compRatio",compRatio);
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
 tiltAmount.setSliderStyle(juce::Slider::LinearHorizontal);
 tiltAmount.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,24);
 tiltAmount.setNumDecimalPlacesToDisplay(1);
 tiltAmountAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"tiltDb",tiltAmount);
 tiltEnabledAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"tiltEnable",tiltEnabled);
 reverbWetAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"reverbMix",reverbWet);
 reverbLengthAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"reverbLength",reverbLength);
 reverbEnableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"reverbEnable",reverbEnabled);
 analyse.onClick=[this]{
  if(auto* p=processor.state.getParameter("listen")){
   p->beginChangeGesture();p->setValueNotifyingHost(1.f);p->endChangeGesture();
  }
 };

 enableAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"enable",enabled);
 processor.getEqResponse(responseCurve);
 const auto initReset=[&](NumericSlider& slider,const char* id){if(auto* p=processor.state.getParameter(id))slider.resetValue=p->convertFrom0to1(p->getDefaultValue());slider.addMouseListener(&slider,true);};
 initReset(matchOutput,"matchOutput");initReset(globalOutput,"globalOutput");initReset(blend,"blend");initReset(depth,"strength");initReset(tiltAmount,"tiltDb");initReset(reverbWet,"reverbMix");initReset(reverbLength,"reverbLength");initReset(compInput,"compInput");initReset(compOutput,"compOutput");initReset(compAttack,"compAttack");initReset(compRelease,"compRelease");initReset(sweetBass,"sweetBass");initReset(sweetTreble,"sweetTreble");initReset(sweetFreq,"sweetFreq");initReset(sweetGain,"sweetGain");initReset(sweetQ,"sweetQ");initReset(sweetSat,"sweetSat");
 const char* ids[]={"Freq","Q","Sens","Range","Attack","Release"};
 const char* suffix[]={" Hz",""," dB"," dB"," ms"," ms"};
 const char* switches[]={"On","Listen","Delta"};
 addAndMakeVisible(dsDual);addAndMakeVisible(dsEnd);
 dsEndA=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"deessEnd",dsEnd);
 dsDualA=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"deessDual",dsDual);
 for(int b=0;b<2;++b){
  const auto prefix=juce::String("ds")+juce::String(b);
  for(int j=0;j<6;++j){auto& sl=dsSliders[b][j];addAndMakeVisible(sl);sl.setSliderStyle(juce::Slider::LinearHorizontal);sl.setTextBoxStyle(juce::Slider::TextBoxRight,false,80,24);sl.setNumDecimalPlacesToDisplay(j==0||j==5?0:j==1?2:1);sl.setTextValueSuffix(suffix[j]);auto id=prefix+ids[j];dsSA[b][j]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,sl);initReset(sl,id.toRawUTF8());}
  for(int j=0;j<3;++j){auto& bt=dsButtons[b][j];bt.setButtonText(juce::String(switches[j]).toUpperCase());addAndMakeVisible(bt);dsBA[b][j]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,prefix+switches[j],bt);}
 }
 addAndMakeVisible(powerEnabled);
 powerAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"powerEnable",powerEnabled);
 for(auto* k:{&sweetBass,&sweetTreble,&sweetFreq,&sweetGain,&sweetQ,&sweetSat,&compInput,&compOutput,&compAttack,&compRelease}){
 k->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k->setTextBoxStyle(juce::Slider::TextBoxBelow,false,80,22);}
 for(auto* k:{&sweetBass,&sweetTreble,&sweetSat,&reverbWet,&reverbLength,&blend,&depth})k->setTextValueSuffix(" %");
 for(auto* k:{&sweetGain,&compInput,&compOutput,&tiltAmount})k->setTextValueSuffix(" dB");
 sweetFreq.setTextValueSuffix(" Hz");compAttack.setTextValueSuffix(" ms");compRelease.setTextValueSuffix(" ms");
 addAndMakeVisible(powerDelta);addAndMakeVisible(powerCut);addAndMakeVisible(rmsMatch);addAndMakeVisible(pluginReset);
 powerDeltaA=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"powerDelta",powerDelta);
 powerCutA=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"powerCut",powerCut);
 rmsMatch.onClick=[this]{processor.startAutoGain();};
 pluginReset.onClick=[this]{juce::Component::SafePointer<DrumBusEditor> safe(this);juce::AlertWindow::showAsync(juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::WarningIcon).withTitle("RESET PLUGIN").withMessage("Reset all parameters and analysis to factory defaults?").withButton("RESET").withButton("CANCEL").withAssociatedComponent(this),[safe](int result){if(result==1&&safe!=nullptr)safe->processor.resetPlugin();});};
 setResizable(true,true);setResizeLimits(1500,920,2400,1400);setSize(1600,920);
 startTimerHz(10);
}

void DrumBusEditor::resized(){
 const int widths[]={280,300,175,220,180,220};
 const int total=1375;
 const bool e=processor.state.getRawParameterValue("deessEnd")->load()>0.5f,r=processor.state.getRawParameterValue("moduleOrder")->load()>0.5f;
 std::vector<int> order{0};if(!e)order.push_back(1);order.push_back(r?3:2);order.push_back(r?2:3);order.push_back(4);order.push_back(5);if(e)order.push_back(1);
 int x=10;const int h=getHeight()-180;
 for(int id:order){int w=(getWidth()-60)*widths[id]/total;panels[id]={x,60,w,h};x+=w+8;}
 auto place=[&](juce::Component& c,int id,int x,int y,int w,int h){auto p=panels[id];c.setBounds(p.getX()+x,p.getY()+y,w,h);};
 auto head=[&](juce::Component& c,int id){place(c,id,panels[id].getWidth()-55,8,45,25);};
 head(enabled,0);head(reverbEnabled,2);head(compEnabled,3);head(sweetEnabled,4);head(powerEnabled,5);
 int w=panels[0].getWidth();place(referenceType,0,12,65,w-24,28);place(analyse,0,12,105,w-24,28);
 juce::Slider* eq[]={&depth,&blend,&tiltAmount,&matchOutput};
 for(int j=0;j<4;++j)place(*eq[j],0,12,360+j*75,w-24,32);
 w=panels[1].getWidth();place(dsEnd,1,10,43,w-20,25);place(dsDual,1,10,75,w-20,25);
 for(int b=0;b<2;++b){int y=110+b*305;for(int j=0;j<6;++j)place(dsSliders[b][j],1,58,y+25+j*36,w-68,18);for(int j=0;j<3;++j)place(dsButtons[b][j],1,10+j*(w-20)/3,y+244,(w-26)/3,23);}
 auto knobs=[&](int id,std::initializer_list<juce::Slider*> list){int j=0;int n=(int)list.size();int top=65;int step=n==2?245:132;for(auto* k:list){place(*k,id,10,top+j*step,panels[id].getWidth()-20,n==2?190:105);++j;}};
 knobs(2,{&reverbWet,&reverbLength});knobs(3,{&compInput,&compOutput,&compAttack,&compRelease});knobs(4,{&sweetBass,&sweetTreble});knobs(5,{&sweetFreq,&sweetGain,&sweetQ,&sweetSat});
 w=panels[3].getWidth();place(compRatio,3,10,600,(w-28)/2,25);place(compMode,3,w/2+4,600,(w-28)/2,25);
 place(powerDelta,5,10,600,panels[5].getWidth()-20,25);place(powerCut,5,10,632,panels[5].getWidth()-20,25);
 pluginReset.setBounds(getWidth()-400,12,150,30);
 moduleOrder.setBounds(getWidth()-240,12,230,30);
 globalOutput.setBounds(getWidth()-340,getHeight()-82,320,25);globalBypass.setBounds(getWidth()-340,getHeight()-40,100,26);rmsMatch.setBounds(getWidth()-232,getHeight()-40,100,26);meterReset.setBounds(getWidth()-124,getHeight()-40,104,26);
}
void DrumBusEditor::paint(juce::Graphics& g){
 auto bg=juce::Colour(0xff101721),panel=juce::Colour(0xff1a2532),cyan=juce::Colour(0xff3dbadf),muted=juce::Colour(0xffd4e4f6);
 g.fillAll(bg);
 auto label=[&](juce::String t,juce::Rectangle<int> r,float size=12.f){g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(size)));g.drawFittedText(t,r,juce::Justification::centred,1);};
 label("LM18 DRUMS | V0.43", {10,8,320,38},23.f);
 const char* names[]={"AUTO EQ","DE-ESSER","AMBIENCE","1176","SWEET ENERGY","POWER EQ"};
 for(int i=0;i<6;++i){auto p=panels[i];g.setColour(panel);g.fillRoundedRectangle(p.toFloat(),5);g.setColour(cyan);g.drawRoundedRectangle(p.toFloat(),5,1);label(names[i],p.withHeight(40).withTrimmedRight(i==1?0:55),15);}
 auto at=[&](int id,int x,int y,int w,int h){return juce::Rectangle<int>(panels[id].getX()+x,panels[id].getY()+y,w,h);};
 auto fmt=[](double v){return juce::String(v,std::abs(v)<1&&v!=0?2:std::abs(v)<10&&v!=std::round(v)?1:0);};
 auto scale=[&](juce::Slider& sl,bool rotary){
 auto b=sl.getBounds();auto lo=sl.getMinimum(),hi=sl.getMaximum();
 g.setColour(muted);g.setFont(juce::Font(juce::FontOptions(10.f)));
 if(rotary){auto cx=b.getCentreX();auto cy=b.getY()+(b.getHeight()-22)*0.5f;float rad=juce::jmin((float)b.getWidth(),(float)b.getHeight()-22)*0.48f;
 for(int i=0;i<=20;++i){float a=juce::MathConstants<float>::pi*(-0.75f+1.5f*i/20.f);float sn=std::sin(a),cs=std::cos(a);g.drawLine(cx+sn*rad,cy-cs*rad,cx+sn*(rad+((i%5==0)?5:2)),cy-cs*(rad+((i%5==0)?5:2)),1);if(i%5==0){auto v=sl.proportionOfLengthToValue(i/20.0);g.drawText(fmt(v),(int)(cx+sn*(rad+13))-20,(int)(cy-cs*(rad+10))-6,40,12,juce::Justification::centred);}}
 }else{int x=b.getX()+8,w=b.getWidth()-sl.getTextBoxWidth()-24,y=b.getBottom();for(int i=0;i<=20;++i){int xx=x+w*i/20;g.drawVerticalLine(xx,(float)y,(float)y+(i%5==0?5:2));if(i%5==0)g.drawText(fmt(sl.proportionOfLengthToValue(i/20.0)),xx-19,y+5,38,12,juce::Justification::centred);}}
 };
 const char* eqNames[]={"DEPTH","BLEND","TILT","OUTPUT"};juce::Slider* eq[]={&depth,&blend,&tiltAmount,&matchOutput};
 for(int j=0;j<4;++j){label(eqNames[j],eq[j]->getBounds().translated(0,-23).withHeight(20));scale(*eq[j],false);}
 auto graph=at(0,12,185,panels[0].getWidth()-24,120).toFloat();g.setColour(bg);g.fillRect(graph);g.setColour(muted);g.drawHorizontalLine((int)graph.getCentreY(),graph.getX(),graph.getRight());
 // Match the processor response grid exactly: 20 Hz to 20 kHz, logarithmic.
 g.setFont(juce::Font(juce::FontOptions(10.f)));
 for(double hz:{20.0,50.0,100.0,200.0,500.0,1000.0,2000.0,5000.0,10000.0,20000.0}){
  const float x=graph.getX()+graph.getWidth()*(float)(std::log(hz/20.0)/std::log(1000.0));
  g.setColour(muted.withAlpha(0.22f));g.drawLine(x,graph.getY(),x,graph.getBottom(),1.f);
  if(hz<20000.0){
   const auto t=hz>=1000.0?juce::String((int)(hz/1000.0))+"k":juce::String((int)hz);
   const int left=juce::jlimit((int)graph.getX(),(int)graph.getRight()-28,(int)x-14);
   g.setColour(muted);g.drawText(t,left,(int)graph.getBottom()+2,28,14,juce::Justification::centred);
  }
 }
 juce::Path path;for(size_t i=0;i<responseCurve.size();++i){float x=graph.getX()+graph.getWidth()*i/(responseCurve.size()-1),y=graph.getCentreY()-juce::jlimit(-30.f,30.f,responseCurve[i])*graph.getHeight()/60.f;if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(cyan);g.strokePath(path,juce::PathStrokeType(2));
 label(processor.matched.load()?"AUTO EQ READY":"ANALYSIS "+juce::String(processor.analysisPercent.load())+" %",at(0,12,325,panels[0].getWidth()-24,20));
 const char* dsNames[]={"Hz","Q","THR","RANGE","ATT","REL"};
 for(int b=0;b<2;++b){int y=110+b*305;label(b?"SIZZLE":"HARSHNESS",at(1,10,y,panels[1].getWidth()-20,20),14);for(int j=0;j<6;++j){label(dsNames[j],at(1,5,y+25+j*36,52,18),10);scale(dsSliders[b][j],false);}for(int c=0;c<2;++c){auto rr=at(1,25,y+273+c*10,panels[1].getWidth()-40,5);g.setColour(bg);g.fillRect(rr);g.setColour(cyan);g.fillRect(rr.withWidth((int)(rr.getWidth()*juce::jlimit(0.f,12.f,processor.deessGR[b][c].load())/12.f)));}}
 auto drawKnobs=[&](std::initializer_list<juce::Slider*> ks,std::initializer_list<const char*> ns){auto it=ns.begin();for(auto* k:ks){label(*it++,k->getBounds().translated(0,-20).withHeight(20));scale(*k,true);}};
 drawKnobs({&reverbWet,&reverbLength},{"DRY / WET","LENGTH"});drawKnobs({&compInput,&compOutput,&compAttack,&compRelease},{"INPUT","OUTPUT","ATTACK","RELEASE"});drawKnobs({&sweetBass,&sweetTreble},{"BASS","TREBLE"});drawKnobs({&sweetFreq,&sweetGain,&sweetQ,&sweetSat},{"FREQ","GAIN","Q","SAT"});
 auto gr=at(3,12,640,panels[3].getWidth()-24,10);g.setColour(bg);g.fillRect(gr);g.setColour(cyan);g.fillRect(gr.withWidth((int)(gr.getWidth()*juce::jlimit(0.f,30.f,processor.compGrVuDb.load())/30.f)));label("GR "+juce::String(grNumericDisplayed,1)+" dB",gr.translated(0,12).withHeight(18));
 int mw=(getWidth()-380)/2;for(int i=0;i<2;++i){int x=10+i*mw,y=getHeight()-78;float vu=i?processor.outputVuDb.load():processor.inputVuDb.load(),peak=i?processor.outputPeakDb.load():processor.inputPeakDb.load(),rms=i?processor.outputRmsDb.load():processor.inputRmsDb.load();label(i?"OUTPUT":"INPUT",{x,y-18,mw-20,15});g.setColour(panel);g.fillRect(x,y,mw-20,14);g.setColour(cyan);g.fillRect(x,y,(int)((mw-20)*juce::jlimit(0.f,1.f,(vu+60)/60)),14);label("RMS "+juce::String(rms,1)+" dBFS | PEAK "+juce::String(peak,1)+" dBFS",{x,y+20,mw-20,22});}
 label("GLOBAL OUTPUT",{getWidth()-340,getHeight()-99,320,15});scale(globalOutput,false);
}
