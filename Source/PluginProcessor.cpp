#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <BinaryData.h>
#include <array>
#include <atomic>
class SR850Processor final : public juce::AudioProcessor {
public:
 SR850Processor() : juce::AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 state(*this,nullptr,"Parameters",{
  std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"strength",1},"Correction %",juce::NormalisableRange<float>(0.f,100.f,1.f),100.f),
  std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gain",1},"Output dB",juce::NormalisableRange<float>(-18.f,12.f,0.1f),0.f),
  std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"profile",1},"Headphone",juce::StringArray{"Samson SR850","KZ AS16 Pro (provisional)"},0)
 }) {}
 const juce::String getName() const override {return "Headphone Reference by Zezo HxCx";}
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
  currentSampleRate.store(rate,std::memory_order_relaxed);
  juce::dsp::ProcessSpec spec{rate,static_cast<juce::uint32>(block),2};
  convolution.reset();
  convolution.loadImpulseResponse(BinaryData::sr850_minphase_wav,BinaryData::sr850_minphase_wavSize,
   juce::dsp::Convolution::Stereo::no,juce::dsp::Convolution::Trim::no,0,
   juce::dsp::Convolution::Normalise::no);
  convolution.prepare(spec);
  kzConvolution.reset();
  kzConvolution.loadImpulseResponse(BinaryData::kz_as16_pro_provisional_wav,BinaryData::kz_as16_pro_provisional_wavSize,
   juce::dsp::Convolution::Stereo::no,juce::dsp::Convolution::Trim::no,0,
   juce::dsp::Convolution::Normalise::no);
  kzConvolution.prepare(spec);
  wet.setSize(2,block);
  kzWet.setSize(2,block);
  profileMix.reset(rate,0.08);
  profileMix.setCurrentAndTargetValue(state.getRawParameterValue("profile")->load());
 }
 void releaseResources() override {convolution.reset(); kzConvolution.reset();}
 bool isBusesLayoutSupported(const BusesLayout& b) const override {
  return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo() &&
         b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
 }
 void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override {
  juce::ScopedNoDenormals noDenormals;
  const int n=buffer.getNumSamples();
  wet.setSize(2,n,false,false,true);
  for(int ch=0;ch<2;++ch) wet.copyFrom(ch,0,buffer,ch,0,n);
  kzWet.setSize(2,n,false,false,true);
  for(int ch=0;ch<2;++ch) kzWet.copyFrom(ch,0,buffer,ch,0,n);
  juce::dsp::AudioBlock<float> kzBlock(kzWet);
  juce::dsp::ProcessContextReplacing<float> kzCtx(kzBlock);
  kzConvolution.process(kzCtx);
  juce::dsp::AudioBlock<float> block(wet);
  juce::dsp::ProcessContextReplacing<float> ctx(block);
  convolution.process(ctx);
  float amount=state.getRawParameterValue("strength")->load()/100.f;
  float gain=juce::Decibels::decibelsToGain(state.getRawParameterValue("gain")->load());
  profileMix.setTargetValue(state.getRawParameterValue("profile")->load());
  for(int i=0;i<n;++i) {
   const float mix=profileMix.getNextValue();
   for(int ch=0;ch<2;++ch) {
    auto* dst=buffer.getWritePointer(ch);
    const float sr=wet.getSample(ch,i);
    const float kz=kzWet.getSample(ch,i);
    const float corrected=sr*(1.f-mix)+kz*mix;
    dst[i]=gain*(dst[i]*(1.f-amount)+corrected*amount);
   }
  }
  const auto* left=buffer.getReadPointer(0);
  const auto* right=buffer.getReadPointer(1);
  for(int i=0;i<n;++i) {
   const auto index=spectrumWrite.fetch_add(1,std::memory_order_relaxed);
   spectrumRing[index%4096].store(0.5f*(left[i]+right[i]),std::memory_order_relaxed);
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
 std::array<std::atomic<float>,4096> spectrumRing{};
 std::atomic<uint64_t> spectrumWrite{0};
 std::atomic<double> currentSampleRate{48000.0};
private:
 juce::dsp::Convolution convolution, kzConvolution;
 juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> profileMix;
 juce::AudioBuffer<float> wet,kzWet;
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

class SR850Editor final : public juce::AudioProcessorEditor, private juce::Timer {
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
  // The KZ profile is visible but deliberately disabled until measured FIR validation.
  profileSelector.addItem("Samson SR850  -  calibrated",1);
  profileSelector.addItem("KZ AS16 Pro  -  provisional",2);
  profileSelector.setSelectedId(1,juce::dontSendNotification);
  profileSelector.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff16110e));
  profileSelector.setColour(juce::ComboBox::textColourId,juce::Colour(0xffead2b3));
  profileSelector.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xffac7b4e));
  profileSelector.setColour(juce::ComboBox::arrowColourId,juce::Colour(0xffd3a674));
  addAndMakeVisible(profileSelector);
  profileAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"profile",profileSelector);
  profileSelector.onChange=[this] { computeCorrectionCurve(); repaint(); };
  computeCorrectionCurve();
  startTimerHz(25);
 }
 ~SR850Editor() override { stopTimer(); strength.setLookAndFeel(nullptr); output.setLookAndFeel(nullptr); }
 void paint(juce::Graphics& g) override {
#if SR850_HAS_ARTWORK
  if(background.isValid()) {
   g.drawImage(background,getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);
   if (isKZSelected()) drawKZTheme(g);
   drawSpectrumPanel(g);
   g.setColour(juce::Colour(0xffe5c49d));
   g.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));
   g.drawText("HEADPHONE PROFILE",615,26,290,22,juce::Justification::centredLeft);
   g.setColour(juce::Colour(0xffd6a36a));
   g.setFont(juce::Font(juce::FontOptions(13.f)));
   g.drawText("By Zezo HxCx",770,603,165,25,juce::Justification::centredRight);
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
  profileSelector.setBounds(615,51,290,31);
#else
  profileSelector.setBounds(520,52,290,31);
#endif
#if SR850_HAS_ARTWORK
  strength.setBounds(71,176,153,150);
  output.setBounds(287,176,153,150);
#else
  strength.setBounds(57,173,170,180);
  output.setBounds(271,173,170,180);
#endif
 }
private:

 bool isKZSelected() const {
  return processor.state.getRawParameterValue("profile")->load() >= .5f;
 }
 void drawKZTheme(juce::Graphics& g) {
  // Self-contained dark-mode artwork fallback until the approved KZ PNG is embedded.
  const juce::Rectangle<float> panel(468.f,94.f,455.f,322.f);
  g.setColour(juce::Colour(0xff090b0c));
  g.fillRoundedRectangle(panel,18.f);
  juce::ColourGradient halo(juce::Colour(0xff4a3524),695.f,142.f,
                            juce::Colour(0xff0b0d0f),730.f,400.f,true);
  g.setGradientFill(halo);
  g.fillRoundedRectangle(panel.reduced(4.f),16.f);
  // Stylized paired IEM shells: this is vector artwork, not a product photograph.
  for(int side=0;side<2;++side) {
   juce::Graphics::ScopedSaveState state(g);
   const float x=side==0?585.f:777.f;
   const float y=side==0?280.f:251.f;
   g.addTransform(juce::AffineTransform::rotation(side==0?-.30f:.30f,x,y));
   g.setColour(juce::Colour(0xff07090a));
   g.fillRoundedRectangle(x-74.f,y-92.f,148.f,184.f,43.f);
   g.setColour(juce::Colour(0xff9b7855));
   g.drawRoundedRectangle(x-74.f,y-92.f,148.f,184.f,43.f,2.f);
   g.setColour(juce::Colour(0xff23282b));
   g.fillRoundedRectangle(x-62.f,y-79.f,124.f,156.f,37.f);
   g.setColour(juce::Colour(0xff454d51));
   for(int row=0;row<8;++row) for(int col=0;col<5;++col) {
    const float xx=x-47.f+col*23.f, yy=y-60.f+row*18.f;
    if(std::pow((xx-x)/57.f,2.f)+std::pow((yy-y)/77.f,2.f)<1.f)
      g.drawLine(xx-6.f,yy-4.f,xx+6.f,yy+4.f,1.f);
   }
   g.setColour(juce::Colour(0xffd1b08c));
   g.setFont(juce::Font(juce::FontOptions(17.f,juce::Font::bold)));
   g.drawText(side==0?"R":"L",x-17.f,y+43.f,34.f,22.f,juce::Justification::centred);
  }
  g.setColour(juce::Colour(0xffe4c29a));
  g.setFont(juce::Font(juce::FontOptions(25.f,juce::Font::bold)));
  g.drawText("KZ AS16 PRO",495,106,385,42,juce::Justification::centred);
  g.setColour(juce::Colour(0xffe1c49e));
  g.setFont(juce::Font(juce::FontOptions(12.f)));
  g.drawText("PROVISIONAL CALIBRATION",500,388,390,20,juce::Justification::centred);
 }
 static constexpr int fftSize=2048;
 juce::dsp::FFT liveFFT{11};
 std::array<float,fftSize*2> fftData{};
 std::array<float,128> spectrumDb{};
 std::array<float,128> correctionDb{};
 uint64_t lastSpectrumWrite=0;
 void computeCorrectionCurve() {
  juce::WavAudioFormat wav;
  const bool kz=processor.state.getRawParameterValue("profile")->load()>=0.5f;
  const void* ir=kz?static_cast<const void*>(BinaryData::kz_as16_pro_provisional_wav):static_cast<const void*>(BinaryData::sr850_minphase_wav);
  const size_t irSize=kz?static_cast<size_t>(BinaryData::kz_as16_pro_provisional_wavSize):static_cast<size_t>(BinaryData::sr850_minphase_wavSize);
  auto* stream=new juce::MemoryInputStream(ir,irSize,false);
  std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(stream,true));
  if(!reader) return;
  const int n=16384;
  juce::AudioBuffer<float> impulse(1,n);
  impulse.clear();
  reader->read(&impulse,0,juce::jmin(n,static_cast<int>(reader->lengthInSamples)),0,true,false);
  juce::dsp::FFT fft(14);
  std::vector<float> data(n*2,0.f);
  for(int i=0;i<n;++i) data[i]=impulse.getSample(0,i);
  fft.performRealOnlyForwardTransform(data.data(),true);
  for(int k=0;k<128;++k) {
   const double hz=20.0*std::pow(1000.0,k/127.0);
   const int bin=juce::jlimit(0,n/2,static_cast<int>(std::round(hz*n/48000.0)));
   const float magnitude=std::hypot(data[2*bin],data[2*bin+1]);
   correctionDb[k]=juce::Decibels::gainToDecibels(juce::jmax(magnitude,1.e-8f),-100.f);
  }
  spectrumDb.fill(-90.f);
 }
 void timerCallback() override {
  const auto write=processor.spectrumWrite.load(std::memory_order_relaxed);
  if(write==lastSpectrumWrite) {
   for(auto& db:spectrumDb) db=juce::jmax(-90.f,db-2.f);
   repaint(48,433,854,113);
   return;
  }
  lastSpectrumWrite=write;
  fftData.fill(0.f);
  for(int i=0;i<fftSize;++i) {
   const auto index=(write-fftSize+static_cast<uint64_t>(i))%4096;
   const float hann=.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(fftSize-1));
   fftData[i]=processor.spectrumRing[index].load(std::memory_order_relaxed)*hann;
  }
  liveFFT.performRealOnlyForwardTransform(fftData.data(),true);
  const double rate=processor.currentSampleRate.load(std::memory_order_relaxed);
  for(int k=0;k<128;++k) {
   const double hz=20.0*std::pow(1000.0,k/127.0);
   const int bin=juce::jlimit(1,fftSize/2,static_cast<int>(std::round(hz*fftSize/rate)));
   const float mag=std::hypot(fftData[2*bin],fftData[2*bin+1])/fftSize*4.f;
   const float db=juce::Decibels::gainToDecibels(juce::jmax(mag,1.e-8f),-100.f);
   spectrumDb[k]=db>spectrumDb[k]?spectrumDb[k]*.35f+db*.65f:spectrumDb[k]*.82f+db*.18f;
  }
  repaint(48,433,854,113);
 }
 void drawSpectrumPanel(juce::Graphics& g) {
  const juce::Rectangle<float> plot(79.f,435.f,815.f,109.f);
  g.setColour(juce::Colour(0xff071313));
  g.fillRect(plot);
  g.setColour(juce::Colour(0x60428a82));
  for(int i=0;i<=10;++i) {
   const float x=plot.getX()+plot.getWidth()*i/10.f;
   g.drawVerticalLine(static_cast<int>(x),plot.getY(),plot.getBottom());
  }
  for(int i=0;i<=4;++i) {
   const float y=plot.getY()+plot.getHeight()*i/4.f;
   g.drawHorizontalLine(static_cast<int>(y),plot.getX(),plot.getRight());
  }
  auto pathFor=[&](const std::array<float,128>& data,float lo,float hi) {
   juce::Path path;
   for(int i=0;i<128;++i) {
    const float x=plot.getX()+plot.getWidth()*i/127.f;
    const float y=juce::jmap(juce::jlimit(lo,hi,data[i]),hi,lo,plot.getY(),plot.getBottom());
    if(i==0) path.startNewSubPath(x,y); else path.lineTo(x,y);
   }
   return path;
  };
  g.setColour(juce::Colour(0xffd6a36a));
  g.strokePath(pathFor(correctionDb,-12.f,12.f),juce::PathStrokeType(1.6f));
  g.setColour(juce::Colour(0xff30e8e2));
  g.strokePath(pathFor(spectrumDb,-75.f,-8.f),juce::PathStrokeType(2.2f));
 }
 SR850Processor& processor;
 VintageKnobs knobs;
 ArtworkKnobs artKnobs;
 juce::Image background;
 juce::Slider strength, output;
 juce::ComboBox profileSelector;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> profileAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> strengthAttachment,outputAttachment;
};
juce::AudioProcessorEditor* SR850Processor::createEditor(){return new SR850Editor(*this);}
