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
  float gain=juce::Decibels::decibelsToGain(state.getRawParameterValue("gain")->load()-7.f);
  for(int ch=0;ch<2;++ch){
   auto* dst=buffer.getWritePointer(ch);
   const auto* corrected=wet.getReadPointer(ch);
   for(int i=0;i<n;++i) dst[i]=gain*(dst[i]*(1.f-amount)+corrected[i]*amount);
  }
 }
 bool hasEditor() const override{return true;}
 juce::AudioProcessorEditor* createEditor() override{return new juce::GenericAudioProcessorEditor(*this);}
 void getStateInformation(juce::MemoryBlock& out) override {
  auto xml=state.copyState().createXml();copyXmlToBinary(*xml,out);
 }
 void setStateInformation(const void* data,int size) override {
  auto xml=getXmlFromBinary(data,size);
  if(xml && xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
 }
private:
 juce::AudioProcessorValueTreeState state;
 juce::dsp::Convolution convolution;
 juce::AudioBuffer<float> wet;
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SR850Processor();}
