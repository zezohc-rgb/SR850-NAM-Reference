#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <BinaryData.h>
class SR850Processor final : public juce::AudioProcessor {
public:
 SR850Processor() : juce::AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 state(*this,nullptr,"Parameters",{
  std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"strength",1},"Correction %",juce::NormalisableRange<float>(0.f,100.f,1.f),100.f),
  std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gain",1},"Output dB",juce::NormalisableRange<float>(-18.f,12.f,0.1f),0.f)
 }) {}
 const juce::String getName() const override {return "Samson SR850 Reference";}
 bool acceptsMidi() const override{return false;}
 bool producesMidi() const override{return false;}
 bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 0.2;}
 int getNumPrograms() override{return 1;}
 int getCurrentProgram() override{return 0;}
 void setCurrentProgram(int) override{}
 const juce::String getProgramName(int) override{return {};}
 void changeProgramName(int,const juce::String&) override{}
 void prepareToPlay(double rate,int block) override {
  juce::dsp::ProcessSpec spec{rate,static_cast<juce::uint32>(block),2};
  convolution.reset();
  convolution.loadImpulseResponse(BinaryData::sr850_minphase_wav,BinaryData::sr850_minphase_wavSize,
   juce::dsp::Convolution::Stereo::no,juce::dsp::Convolution::Trim::no,0,
   juce::dsp::Convolution::Normalise::no);
  convolution.prepare(spec);
  wet.setSize(2,block);
 }
 void releaseResources() override {convolution.reset();}
 bool isBusesLayoutSupported(const BusesLayout& b) const override {
  return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo() &&
         b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
 }
 void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override {
  juce::ScopedNoDenormals noDenormals;
  const int n=buffer.getNumSamples();
  wet.setSize(2,n,false,false,true);
  for(int ch=0;ch<2;++ch) wet.copyFrom(ch,0,buffer,ch,0,n);
  juce::dsp::AudioBlock<float> block(wet);
  juce::dsp::ProcessContextReplacing<float> ctx(block);
  convolution.process(ctx);
  float amount=state.getRawParameterValue("strength")->load()/100.f;
  float gain=juce::Decibels::decibelsToGain(state.getRawParameterValue("gain")->load());
  for(int ch=0;ch<2;++ch){
   auto* dst=buffer.getWritePointer(ch);
   const auto* corrected=wet.getReadPointer(ch);
   for(int i=0;i<n;++i) dst[i]=gain*(dst[i]*(1.f-amount)+corrected[i]*amount);
  }
 }
 bool hasEditor() const override{return true;}
 juce::AudioProcessorEditor* createEditor() override;
 void getStateInformation(juce::MemoryBlock& out) override {
  auto xml=state.copyState().createXml();copyXmlToBinary(*xml,out);
 }
 void setStateInformation(const void* data,int size) override {
  auto xml=getXmlFromBinary(data,size);
  if(xml && xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
 }
public:
 juce::AudioProcessorValueTreeState state;
private:
 juce::dsp::Convolution convolution;
 juce::AudioBuffer<float> wet;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SR850Processor();}

class VintageKnobs final : public juce::LookAndFeel_V4 {
public:
 void drawRotarySlider(juce::Graphics& g, int x,int y,int width,int height,
                      float pos,float start,float end,juce::Slider&) override {
  const auto cx=x+width*0.5f, cy=y+height*0.5f;
  const auto radius=juce::jmin(width,height)*0.37f;
  const auto angle=start+pos*(end-start);
  for(int i=0;i<=24;++i) {
   const float a=start+(end-start)*i/24.f;
   const auto r1=radius*1.18f, r2=radius*(i%6==0?1.40f:1.31f);
   g.setColour(juce::Colour(i%6==0?0xffb99261:0xff6b5843));
   g.drawLine(cx+std::sin(a)*r1,cy-std::cos(a)*r1,
              cx+std::sin(a)*r2,cy-std::cos(a)*r2,i%6==0?1.7f:0.8f);
  }
  g.setColour(juce::Colour(0xff070707)); g.fillEllipse(cx-radius-5,cy-radius-5,2*(radius+5),2*(radius+5));
  juce::ColourGradient metal(juce::Colour(0xffc49a65),cx-radius,cy-radius,
                             juce::Colour(0xff33251b),cx+radius,cy+radius,false);
  g.setGradientFill(metal); g.fillEllipse(cx-radius,cy-radius,2*radius,2*radius);
  g.setColour(juce::Colour(0xffd2aa74)); g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1.8f);
  for(int i=0;i<8;++i) {
   const auto r=radius*(0.16f+i*0.10f);
   g.setColour(juce::Colour(0x268f795f)); g.drawEllipse(cx-r,cy-r,2*r,2*r,0.65f);
  }
  g.setColour(juce::Colour(0xffe7dfcf));
  g.drawLine(cx+std::sin(angle)*radius*.20f,cy-std::cos(angle)*radius*.20f,
             cx+std::sin(angle)*radius*.84f,cy-std::cos(angle)*radius*.84f,3.5f);
 }
};

class ArtworkKnobs final : public juce::LookAndFeel_V4 {
public:
 void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
  const float cx=x+w*0.5f, cy=y+h*0.5f;
  // Opaque bronze face covers the static pointer printed in the background.
  const float radius=juce::jmin(w,h)*0.375f;
  g.setColour(juce::Colour(0xff100c09));
  g.fillEllipse(cx-radius-2.f,cy-radius-2.f,2.f*radius+4.f,2.f*radius+4.f);
  juce::ColourGradient bronze(juce::Colour(0xffe5b878),cx-radius,cy-radius,
                              juce::Colour(0xff49301d),cx+radius,cy+radius,false);
  g.setGradientFill(bronze);
  g.fillEllipse(cx-radius,cy-radius,2.f*radius,2.f*radius);
  g.setColour(juce::Colour(0xffc99559));
  g.drawEllipse(cx-radius,cy-radius,2.f*radius,2.f*radius,2.f);
  for(int ring=1;ring<=8;++ring) {
   const float rr=radius*(0.13f+ring*0.1f);
   g.setColour(juce::Colour(0x30634226));
   g.drawEllipse(cx-rr,cy-rr,2.f*rr,2.f*rr,0.7f);
  }
  const float angle=start+pos*(end-start);
  const float sn=std::sin(angle),cs=std::cos(angle);
  g.setColour(juce::Colour(0xff170f09));
  g.drawLine(cx+sn*radius*.18f,cy-cs*radius*.18f,
             cx+sn*radius*.86f,cy-cs*radius*.86f,5.f);
  g.setColour(juce::Colour(0xffffe1aa));
  g.drawLine(cx+sn*radius*.24f,cy-cs*radius*.24f,
             cx+sn*radius*.82f,cy-cs*radius*.82f,2.f);
 }
};

class SR850Editor final : public juce::AudioProcessorEditor {
public:
 explicit SR850Editor(SR850Processor& p) : juce::AudioProcessorEditor(p), processor(p) {
#if SR850_HAS_ARTWORK
  background=juce::ImageFileFormat::loadFrom(BinaryData::sr850_gui_png,BinaryData::sr850_gui_pngSize);
  setSize(960, 640);
#else
  setSize(860, 540);
#endif
  for (auto* slider : {&strength, &output}) {
#if SR850_HAS_ARTWORK
   slider->setLookAndFeel(&artKnobs);
   slider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
#else
   slider->setLookAndFeel(&knobs);
   slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 96, 24);
#endif
   slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   slider->setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff78d9d1));
   slider->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff273e45));
   slider->setColour(juce::Slider::thumbColourId, juce::Colour(0xffc5ffed));
   slider->setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffd9e6e8));
   slider->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
   addAndMakeVisible(*slider);
  }
  strengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"strength",strength);
  outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,"gain",output);
 }
 ~SR850Editor() override { strength.setLookAndFeel(nullptr); output.setLookAndFeel(nullptr); }
 void paint(juce::Graphics& g) override {
#if SR850_HAS_ARTWORK
  if(background.isValid()) {
   g.drawImage(background,getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);
   return;
  }
#endif
  g.fillAll(juce::Colour(0xff0a0706));
  juce::ColourGradient bg(juce::Colour(0xff2d1b11),0,0,juce::Colour(0xff080909),860,540,false);
  g.setGradientFill(bg); g.fillRoundedRectangle(8.f,8.f,844.f,524.f,17.f);
  g.setColour(juce::Colour(0xff9d6d3f)); g.drawRoundedRectangle(8.f,8.f,844.f,524.f,17.f,2.f);
  g.setColour(juce::Colour(0xffb99568));
  g.setFont(juce::Font(juce::FontOptions(34.f,juce::Font::bold)));
  g.drawText("SAMSON",42,24,285,48,juce::Justification::left);
  g.setFont(juce::Font(juce::FontOptions(33.f)));
  g.drawText("SR850",330,24,165,48,juce::Justification::left);
  g.setColour(juce::Colour(0xffc4a786)); g.setFont(juce::Font(juce::FontOptions(13.f)));
  g.drawText("ACCURATE MONITORING  /  REAL SOUND",522,30,290,30,juce::Justification::right);
  g.setColour(juce::Colour(0xff8e5b33)); g.drawLine(22.f,86.f,838.f,86.f,2.f);

  // Dark bronze silhouette echoing the supplied vintage SR850 reference.
  g.setColour(juce::Colour(0xff13100e)); g.fillRoundedRectangle(476.f,102.f,344.f,265.f,20.f);
  juce::Path arch; arch.startNewSubPath(530.f,285.f);
  arch.cubicTo(510.f,80.f,797.f,45.f,800.f,285.f);
  g.setColour(juce::Colour(0xff513a2a)); g.strokePath(arch,juce::PathStrokeType(31.f));
  g.setColour(juce::Colour(0xffae7947)); g.strokePath(arch,juce::PathStrokeType(2.f));
  g.setColour(juce::Colour(0xff090909)); g.fillEllipse(574.f,123.f,230.f,230.f);
  g.setColour(juce::Colour(0xff6b4a30)); g.drawEllipse(574.f,123.f,230.f,230.f,5.f);
  for(int row=0;row<12;++row) for(int col=0;col<12;++col) {
   const float xx=593.f+col*16.f, yy=142.f+row*16.f;
   if(std::pow(xx-689.f,2.f)+std::pow(yy-238.f,2.f)<9200.f) {
    g.setColour(juce::Colour(0xff30251d)); g.fillEllipse(xx,yy,6.f,6.f);
   }
  }
  g.setColour(juce::Colour(0xff17110e)); g.fillEllipse(641.f,188.f,105.f,105.f);
  g.setColour(juce::Colour(0xff916c49)); g.drawEllipse(641.f,188.f,105.f,105.f,3.f);
  g.setColour(juce::Colour(0xffc2a17c)); g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
  g.drawText("SR850",654,227,80,25,juce::Justification::centred);

  g.setColour(juce::Colour(0xffe0bd8e)); g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
  g.drawText("CORRECTION",56,112,172,28,juce::Justification::centred);
  g.drawText("OUTPUT",270,112,172,28,juce::Justification::centred);
  g.setColour(juce::Colour(0xffb79b79)); g.setFont(juce::Font(juce::FontOptions(12.f)));
  g.drawText("INTENSITY",56,139,172,22,juce::Justification::centred);
  g.drawText("GAIN",270,139,172,22,juce::Justification::centred);

  g.setColour(juce::Colour(0xff22160f)); g.fillRoundedRectangle(35.f,380.f,790.f,116.f,9.f);
  g.setColour(juce::Colour(0xff996b3d)); g.drawRoundedRectangle(35.f,380.f,790.f,116.f,9.f,1.5f);
  g.setColour(juce::Colour(0xff64d8d5));
  juce::Path curve; curve.startNewSubPath(65.f,452.f);
  for(int i=1;i<=150;++i) {
   const float x=65.f+i*4.85f;
   const float t=i/150.f;
   const float y=452.f-23.f*std::sin(juce::MathConstants<float>::pi*t)
                     -5.f*std::sin(15.f*t)-3.f*std::sin(40.f*t);
   curve.lineTo(x,y);
  }
  for(int i=0;i<=12;++i) { g.setColour(juce::Colour(0x2256aaa4)); g.drawVerticalLine(65+i*60,398.f,477.f); }
  g.setColour(juce::Colour(0xff62d6d2)); g.strokePath(curve,juce::PathStrokeType(2.f));
  g.setColour(juce::Colour(0xffa78b6b)); g.setFont(juce::Font(juce::FontOptions(12.f)));
  g.drawText("20 Hz",56,470,80,20,juce::Justification::left);
  g.drawText("20 kHz",730,470,80,20,juce::Justification::right);
  g.drawText("CORRECTION  /  AUTOEQ (RTINGS)",42,505,430,20,juce::Justification::left);
 }

 void resized() override {
#if SR850_HAS_ARTWORK
  strength.setBounds(71,176,153,150);
  output.setBounds(287,176,153,150);
#else
  strength.setBounds(57,173,170,180);
  output.setBounds(271,173,170,180);
#endif
 }
private:
 SR850Processor& processor;
 VintageKnobs knobs;
 ArtworkKnobs artKnobs;
 juce::Image background;
 juce::Slider strength, output;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strengthAttachment,outputAttachment;
};
juce::AudioProcessorEditor* SR850Processor::createEditor(){return new SR850Editor(*this);}
