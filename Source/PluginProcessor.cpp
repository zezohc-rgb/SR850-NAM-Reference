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
 const juce::String getName() const override {return "SR850 NAM Reference";}
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

class SR850Editor final : public juce::AudioProcessorEditor {
public:
 explicit SR850Editor(SR850Processor& p) : juce::AudioProcessorEditor(p), processor(p) {
  setSize(720, 420);
  for (auto* slider : {&strength, &output}) {
   slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 96, 24);
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
 void paint(juce::Graphics& g) override {
  auto bounds = getLocalBounds().toFloat();
  g.fillAll(juce::Colour(0xff0b1017));
  juce::ColourGradient bg(juce::Colour(0xff172a32),0.f,0.f,juce::Colour(0xff090d13),720.f,420.f,false);
  g.setGradientFill(bg); g.fillRect(bounds);
  g.setColour(juce::Colour(0x332cc6b8));
  for(int i=0;i<10;++i) g.drawVerticalLine(40+i*72,0.f,420.f);
  g.setColour(juce::Colour(0xff83ddd2));
  g.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));
  g.drawText("SR850  /  NAM REFERENCE",38,26,440,22,juce::Justification::left);
  g.setColour(juce::Colour(0xfff1f7f7));
  g.setFont(juce::Font(juce::FontOptions(31.f,juce::Font::bold)));
  g.drawText("REFERENCE MONITOR",36,64,520,43,juce::Justification::left);
  g.setColour(juce::Colour(0xff91a9b0));
  g.setFont(juce::Font(juce::FontOptions(13.f)));
  g.drawText("Samson SR850  |  Full-resolution minimum-phase FIR",39,112,520,23,juce::Justification::left);

  // Low-light vector illustration inspired by the SR850 headphone silhouette.
  juce::Graphics::ScopedSaveState saved(g);
  g.addTransform(juce::AffineTransform::translation(480.f,120.f));
  juce::Path arch;
  arch.startNewSubPath(8.f,108.f);
  arch.cubicTo(1.f, -40.f, 202.f,-48.f, 206.f,108.f);
  g.setColour(juce::Colour(0x5548a69e)); g.strokePath(arch,juce::PathStrokeType(23.f));
  g.setColour(juce::Colour(0xff28343b)); g.strokePath(arch,juce::PathStrokeType(14.f));
  g.setColour(juce::Colour(0xff53646a)); g.strokePath(arch,juce::PathStrokeType(2.f));
  for (int side=0;side<2;++side) {
   auto x=side==0 ? 0.f : 169.f;
   g.setColour(juce::Colour(0xff131e24)); g.fillRoundedRectangle(x,93.f,45.f,119.f,18.f);
   g.setColour(juce::Colour(0xff607b7d)); g.drawRoundedRectangle(x,93.f,45.f,119.f,18.f,2.f);
   g.setColour(juce::Colour(0xff24343a)); g.fillRoundedRectangle(x+8.f,105.f,29.f,94.f,12.f);
   g.setColour(juce::Colour(0x9985d9ce)); g.drawRoundedRectangle(x+8.f,105.f,29.f,94.f,12.f,1.f);
  }
  g.setColour(juce::Colour(0xff93aaa9)); g.setFont(juce::Font(juce::FontOptions(11.f,juce::Font::bold)));
  g.drawText("SR850",73,160,70,20,juce::Justification::centred);

  g.setColour(juce::Colour(0x663f7e80)); g.drawLine(38.f,280.f,676.f,280.f,1.f);
  g.setColour(juce::Colour(0xffbed3d4)); g.setFont(juce::Font(juce::FontOptions(12.f)));
  g.drawText("CORRECTION",43,239,145,22,juce::Justification::centred);
  g.drawText("OUTPUT LEVEL",224,239,145,22,juce::Justification::centred);
  g.setColour(juce::Colour(0xff6f9296)); g.drawText("AUTOEQ  /  695 MEASURED POINTS",39,378,410,20,juce::Justification::left);
  g.drawText("VST3   •   STEREO",525,378,160,20,juce::Justification::right);
 }
 void resized() override {
  strength.setBounds(47,270,136,99);
  output.setBounds(229,270,136,99);
 }
private:
 SR850Processor& processor;
 juce::Slider strength, output;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strengthAttachment,outputAttachment;
};
juce::AudioProcessorEditor* SR850Processor::createEditor(){return new SR850Editor(*this);}
