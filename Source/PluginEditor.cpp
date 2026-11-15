#include "PluginEditor.h"

static juce::Image createTintedImage(const juce::Image& source, juce::Colour tint)
{
    if (!source.isValid()) return source;
    juce::Image result(juce::Image::ARGB, source.getWidth(), source.getHeight(), true);
    juce::Image::BitmapData srcData(source, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dstData(result, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < source.getHeight(); ++y) {
        for (int x = 0; x < source.getWidth(); ++x) {
            juce::Colour c = srcData.getPixelColour(x, y);
            juce::uint8 r = (juce::uint8)((c.getRed() * tint.getRed()) / 255);
            juce::uint8 g = (juce::uint8)((c.getGreen() * tint.getGreen()) / 255);
            juce::uint8 b = (juce::uint8)((c.getBlue() * tint.getBlue()) / 255);
            dstData.setPixelColour(x, y, juce::Colour(r, g, b).withAlpha(c.getAlpha()));
        }
    }
    return result;
}

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    startTimerHz(30);

    addAndMakeVisible(vuMeter_L);
    addAndMakeVisible(vuMeter_R);

    // VU Переключатели
    vuSwitch_L.addItemList({"GR", "OUT 0dB", "OUT +10dB"}, 1);
    vuSwitch_L.setSelectedId(1);
    vuSwitch_L.onChange = [this]() { vuMeter_L.setMode(vuSwitch_L.getSelectedId() - 1); };
    addAndMakeVisible(vuSwitch_L);

    vuSwitch_R.addItemList({"GR", "OUT 0dB", "OUT +10dB"}, 1);
    vuSwitch_R.setSelectedId(1);
    vuSwitch_R.onChange = [this]() { vuMeter_R.setMode(vuSwitch_R.getSelectedId() - 1); };
    addAndMakeVisible(vuSwitch_R);

    // Пресеты
    presetBox.addItem("Default", 1);
    presetBox.addItem("Vocal Bus", 2);
    presetBox.addItem("Drum Bus", 3);
    presetBox.addItem("Keys Bus", 4);
    presetBox.setSelectedId(1);
    presetBox.onChange = [this]() { 
        int id = presetBox.getSelectedId();
        auto setP = [this](const juce::String& paramId, float value) {
            auto* param = audioProcessor.apvts.getParameter(paramId);
            param->setValueNotifyingHost(param->convertTo0to1(value));
        };
        auto setC = [this](const juce::String& paramId, int index) {
            auto* param = audioProcessor.apvts.getParameter(paramId);
            param->setValueNotifyingHost(param->convertTo0to1((float)index));
        };

        if (id == 2) {
            setP("thresholdL", -15.0f); setP("thresholdR", -15.0f);
            setC("attackL", 2); setC("attackR", 2);
            setC("releaseL", 2); setC("releaseR", 2);
            setC("biasL", 1); setC("biasR", 1);
            setC("sidechainEqL", 1); setC("sidechainEqR", 1);
            setP("stereoWidth", 1.2f);
            setP("mix", 1.0f);
        } else if (id == 3) {
            setP("thresholdL", -10.0f); setP("thresholdR", -10.0f);
            setC("attackL", 0); setC("attackR", 0);
            setC("releaseL", 1); setC("releaseR", 1);
            setC("biasL", 2); setC("biasR", 2);
            setC("sidechainEqL", 0); setC("sidechainEqR", 0);
            setP("stereoWidth", 1.0f);
            setP("mix", 0.8f);
        } else if (id == 4) {
            setP("thresholdL", -20.0f); setP("thresholdR", -20.0f);
            setC("attackL", 3); setC("attackR", 3);
            setC("releaseL", 3); setC("releaseR", 3);
            setC("biasL", 0); setC("biasR", 0);
            setC("airBassL", 1); setC("airBassR", 1);
            setP("stereoWidth", 1.5f);
            setP("mix", 1.0f);
        } else {
            setP("thresholdL", -10.0f); setP("thresholdR", -10.0f);
            setC("attackL", 2); setC("attackR", 2);
            setC("releaseL", 2); setC("releaseR", 2);
            setC("biasL", 0); setC("biasR", 0);
            setC("sidechainEqL", 0); setC("sidechainEqR", 0);
            setC("airBassL", 0); setC("airBassR", 0);
            setP("stereoWidth", 1.0f);
            setP("mix", 1.0f);
        }
    };
    addAndMakeVisible(presetBox);

    // TMT
    addAndMakeVisible(tmtEnableToggle);
    addAndMakeVisible(tmtModeButton);
    addAndMakeVisible(tmtLeftBtn);
    addAndMakeVisible(tmtRightBtn);
    addAndMakeVisible(tmtRandomBtn);
    addAndMakeVisible(tmtDisplay);

    tmtDisplay.setJustificationType(juce::Justification::centred);
    tmtDisplay.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    tmtDisplay.setColour(juce::Label::backgroundColourId, juce::Colour(0xff111111));
    tmtDisplay.setColour(juce::Label::textColourId, juce::Colour(0xffcc0000));
    
    attTMTMode = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "tmtStereoMode", tmtModeButton);
    attTMTEnable = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "tmtEnable", tmtEnableToggle);

    tmtModeButton.onClick = [this]() {
        bool isAnalog = audioProcessor.apvts.getRawParameterValue("tmtStereoMode")->load() < 0.5f;
        tmtModeButton.setButtonText(isAnalog ? "D" : "A");
    };
    
    tmtLeftBtn.onClick = [this]() {
        auto* param = audioProcessor.apvts.getParameter("tmtChannel");
        int current = static_cast<int>(param->getValue() * 9);
        current = juce::jmax(0, current - 1);
        param->setValueNotifyingHost(param->convertTo0to1((float)current));
    };

    tmtRightBtn.onClick = [this]() {
        auto* param = audioProcessor.apvts.getParameter("tmtChannel");
        int current = static_cast<int>(param->getValue() * 9);
        current = juce::jmin(9, current + 1);
        param->setValueNotifyingHost(param->convertTo0to1((float)current));
    };

    tmtRandomBtn.onClick = [this]() {
        auto* param = audioProcessor.apvts.getParameter("tmtChannel");
        int randomVal = juce::Random::getSystemRandom().nextInt(10);
        param->setValueNotifyingHost(param->convertTo0to1((float)randomVal));
    };

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    auto setupSteppedSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name, int numSteps) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(0.0, (double)(numSteps - 1), 1.0);
        s.setLookAndFeel(&steppedLnf);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    // Левый канал
    setupSlider(thresholdSlider_L, labelThreshold_L, "THRESHOLD L");
    setupSlider(inputGainSlider_L, labelInput_L, "INPUT L");
    setupSlider(outputGainSlider_L, labelOutput_L, "OUTPUT L");
    setupSteppedSlider(attackSlider_L, labelAttack_L, "ATTACK L", 6);
    setupSteppedSlider(releaseSlider_L, labelRelease_L, "RELEASE L", 6);
    setupSteppedSlider(biasSlider_L, labelBias_L, "BIAS L", 3);
    setupSteppedSlider(rectifierSlider_L, labelRectifier_L, "RECTIFIER L", 6);
    setupSteppedSlider(airBassSlider_L, labelAirBass_L, "AIR/TAPE L", 3);
    setupSteppedSlider(sidechainSlider_L, labelSidechain_L, "SC EQ L", 5);

    // Правый канал
    setupSlider(thresholdSlider_R, labelThreshold_R, "THRESHOLD R");
    setupSlider(inputGainSlider_R, labelInput_R, "INPUT R");
    setupSlider(outputGainSlider_R, labelOutput_R, "OUTPUT R");
    setupSteppedSlider(attackSlider_R, labelAttack_R, "ATTACK R", 6);
    setupSteppedSlider(releaseSlider_R, labelRelease_R, "RELEASE R", 6);
    setupSteppedSlider(biasSlider_R, labelBias_R, "BIAS R", 3);
    setupSteppedSlider(rectifierSlider_R, labelRectifier_R, "RECTIFIER R", 6);
    setupSteppedSlider(airBassSlider_R, labelAirBass_R, "AIR/TAPE R", 3);
    setupSteppedSlider(sidechainSlider_R, labelSidechain_R, "SC EQ R", 5);

    // Мастер
    setupSlider(mixSlider, labelMix, "MIX");
    setupSlider(widthSlider, labelWidth, "WIDTH");
    setupSlider(monoSlider, labelMono, "MONO");
    setupSlider(headroomSlider, labelHeadroom, "HEADROOM");

    linkButton.setLookAndFeel(&toggleLnf);
    midSideButton.setLookAndFeel(&toggleLnf);
    tmtEnableToggle.setLookAndFeel(&toggleLnf);

    addAndMakeVisible(linkButton);
    addAndMakeVisible(midSideButton);

    // Attachments
    attThreshold_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdL", thresholdSlider_L);
    attInput_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainL", inputGainSlider_L);
    attOutput_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGainL", outputGainSlider_L);
    attAttack_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attackL", attackSlider_L);
    attRelease_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "releaseL", releaseSlider_L);
    attBias_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "biasL", biasSlider_L);
    attRectifier_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "rectifierL", rectifierSlider_L);
    attAirBass_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "airBassL", airBassSlider_L);
    attSidechain_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "sidechainEqL", sidechainSlider_L);

    attThreshold_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdR", thresholdSlider_R);
    attInput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainR", inputGainSlider_R);
    attOutput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGainR", outputGainSlider_R);
    attAttack_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attackR", attackSlider_R);
    attRelease_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "releaseR", releaseSlider_R);
    attBias_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "biasR", biasSlider_R);
    attRectifier_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "rectifierR", rectifierSlider_R);
    attAirBass_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "airBassR", airBassSlider_R);
    attSidechain_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "sidechainEqR", sidechainSlider_R);

    attMix = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    attWidth = std::make_unique<SliderAttachment>(audioProcessor.apvts, "stereoWidth", widthSlider);
    attMono = std::make_unique<SliderAttachment>(audioProcessor.apvts, "monoMaker", monoSlider);
    attHeadroom = std::make_unique<SliderAttachment>(audioProcessor.apvts, "headroom", headroomSlider);
    attLink = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "link", linkButton);
    attMidSide = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "midSide", midSideButton);

    // ФИКС LINK
    linkButton.onClick = [this]() {
        if (!linkButton.getToggleState()) {
            auto copyParam = [&](const juce::String& idL, const juce::String& idR) {
                float val = audioProcessor.apvts.getRawParameterValue(idL)->load();
                auto* paramR = audioProcessor.apvts.getParameter(idR);
                paramR->setValueNotifyingHost(paramR->convertTo0to1(val));
            };
            copyParam("thresholdL", "thresholdR");
            copyParam("inputGainL", "inputGainR");
            copyParam("outputGainL", "outputGainR");
            
            auto copyChoice = [&](const juce::String& idL, const juce::String& idR) {
                int idx = static_cast<int>(audioProcessor.apvts.getRawParameterValue(idL)->load());
                auto* paramR = audioProcessor.apvts.getParameter(idR);
                paramR->setValueNotifyingHost(paramR->convertTo0to1((float)idx));
            };
            copyChoice("biasL", "biasR");
            copyChoice("rectifierL", "rectifierR");
            copyChoice("attackL", "attackR");
            copyChoice("releaseL", "releaseR");
            copyChoice("airBassL", "airBassR");
            copyChoice("sidechainEqL", "sidechainEqR");
        }
    };

    setSize (1100, 750);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() 
{ 
    setLookAndFeel(nullptr); 
}

void IronCompressorAudioProcessorEditor::timerCallback()
{
    int modeL = vuSwitch_L.getSelectedId() - 1;
    int modeR = vuSwitch_R.getSelectedId() - 1;

    if (modeL == 0) vuMeter_L.setLevel(audioProcessor.currentGR_dB_L.load());
    else vuMeter_L.setLevel(audioProcessor.currentOut_dB_L.load());

    if (modeR == 0) vuMeter_R.setLevel(audioProcessor.currentGR_dB_R.load());
    else vuMeter_R.setLevel(audioProcessor.currentOut_dB_R.load());

    vuMeter_L.tick();
    vuMeter_R.tick();

    bool isTMTEnabled = audioProcessor.apvts.getRawParameterValue("tmtEnable")->load() > 0.5f;
    bool isLink = audioProcessor.apvts.getRawParameterValue("link")->load() > 0.5f;
    
    thresholdSlider_R.setEnabled(!isLink);
    inputGainSlider_R.setEnabled(!isLink);
    outputGainSlider_R.setEnabled(!isLink);
    biasSlider_R.setEnabled(!isLink);
    rectifierSlider_R.setEnabled(!isLink);
    attackSlider_R.setEnabled(!isLink);
    releaseSlider_R.setEnabled(!isLink);
    airBassSlider_R.setEnabled(!isLink);
    sidechainSlider_R.setEnabled(!isLink);

    if (isLink) {
        thresholdSlider_R.setValue(thresholdSlider_L.getValue(), juce::dontSendNotification);
        inputGainSlider_R.setValue(inputGainSlider_L.getValue(), juce::dontSendNotification);
        outputGainSlider_R.setValue(outputGainSlider_L.getValue(), juce::dontSendNotification);
        biasSlider_R.setValue(biasSlider_L.getValue(), juce::dontSendNotification);
        rectifierSlider_R.setValue(rectifierSlider_L.getValue(), juce::dontSendNotification);
        attackSlider_R.setValue(attackSlider_L.getValue(), juce::dontSendNotification);
        releaseSlider_R.setValue(releaseSlider_L.getValue(), juce::dontSendNotification);
        airBassSlider_R.setValue(airBassSlider_L.getValue(), juce::dontSendNotification);
        sidechainSlider_R.setValue(sidechainSlider_L.getValue(), juce::dontSendNotification);
    }

    tmtModeButton.setEnabled(isTMTEnabled);
    tmtLeftBtn.setEnabled(isTMTEnabled);
    tmtRightBtn.setEnabled(isTMTEnabled);
    tmtRandomBtn.setEnabled(isTMTEnabled);
    tmtDisplay.setAlpha(isTMTEnabled ? 1.0f : 0.4f);

    if (isTMTEnabled) {
        auto* param = audioProcessor.apvts.getParameter("tmtChannel");
        int current = static_cast<int>(param->getValue() * 9);
        bool isDigital = audioProcessor.apvts.getRawParameterValue("tmtStereoMode")->load() > 0.5f;
        
        tmtModeButton.setButtonText(isDigital ? "A" : "D");
        
        if (isDigital) {
            tmtDisplay.setText(juce::String(current * 2 + 1) + "-" + juce::String(current * 2 + 1), juce::dontSendNotification);
        } else {
            tmtDisplay.setText(juce::String(current * 2 + 1) + "-" + juce::String(current * 2 + 2), juce::dontSendNotification);
        }
    } else {
        tmtDisplay.setText("OFF", juce::dontSendNotification);
    }
}

void IronCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    static juce::Image bgTextureOriginal = juce::ImageFileFormat::loadFrom(
        BinaryData::bg_texture_jpg, 
        BinaryData::bg_texture_jpgSize
    );
    static juce::Image bgTextureTinted = createTintedImage(bgTextureOriginal, juce::Colour(0xffb30000));

    if (bgTextureTinted.isValid())
        g.drawImage(bgTextureTinted, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
    else
        g.fillAll (juce::Colours::orange);

    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (0, 0, getWidth(), 60);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("SPL IRON", 20, 10, 200, 40, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffaaaaaa));
    g.setFont (14.0f);
    g.drawText ("Parallel Dual-Tube Compressor", 160, 15, 300, 30, juce::Justification::centredLeft);
    
    g.setColour(juce::Colour(0x44000000));
    g.fillRoundedRectangle(210.0f, 70.0f, 300.0f, 120.0f, 5.0f);
    g.fillRoundedRectangle(590.0f, 70.0f, 300.0f, 120.0f, 5.0f);
}

void IronCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (60);

    presetBox.setBounds (getWidth() - 170, 15, 150, 30);

    const int smallKnob = 60;
    const int bigKnob = 90;
    const int labelHeight = 16;

    // VU и переключатели
    vuMeter_L.setBounds (210, 70, 300, 90);
    vuSwitch_L.setBounds (315, 165, 90, 22);

    vuMeter_R.setBounds (590, 70, 300, 90);
    vuSwitch_R.setBounds (695, 165, 90, 22);

    // TMT
    int cx = 460; 
    tmtEnableToggle.setBounds(cx + 10, 130, 80, 24);
    tmtModeButton.setBounds (cx + 10, 160, 40, 24);
    tmtLeftBtn.setBounds (cx + 55, 160, 30, 24);
    tmtDisplay.setBounds (cx + 90, 160, 60, 24);
    tmtRightBtn.setBounds (cx + 155, 160, 30, 24);
    tmtRandomBtn.setBounds (cx + 190, 160, 40, 24);

    // Левый канал
    int lx = 20;
    labelInput_L.setBounds (lx, 80, 80, labelHeight);
    inputGainSlider_L.setBounds (lx, 100, 80, smallKnob + 20);
    
    labelThreshold_L.setBounds (350, 210, 100, labelHeight);
    thresholdSlider_L.setBounds (330, 230, bigKnob + 40, bigKnob + 40);

    labelAttack_L.setBounds (120, 210, 80, labelHeight);
    attackSlider_L.setBounds (120, 230, 80, smallKnob + 20);
    
    labelRelease_L.setBounds (120, 310, 80, labelHeight);
    releaseSlider_L.setBounds (120, 330, 80, smallKnob + 20);

    labelBias_L.setBounds (lx, 340, 80, labelHeight);
    biasSlider_L.setBounds (lx, 360, 80, smallKnob + 20);
    
    labelRectifier_L.setBounds (130, 430, 150, labelHeight);
    rectifierSlider_L.setBounds (130, 450, 80, smallKnob + 20);

    labelOutput_L.setBounds (lx, 460, 80, labelHeight);
    outputGainSlider_L.setBounds (lx, 480, 80, smallKnob + 20);

    labelSidechain_L.setBounds (250, 420, 100, labelHeight);
    sidechainSlider_L.setBounds (250, 440, 80, smallKnob + 20);
    
    labelAirBass_L.setBounds (250, 520, 100, labelHeight);
    airBassSlider_L.setBounds (250, 540, 80, smallKnob + 20);

    // Правый канал
    int rx = 1000;
    labelInput_R.setBounds (rx, 80, 80, labelHeight);
    inputGainSlider_R.setBounds (rx, 100, 80, smallKnob + 20);
    
    labelThreshold_R.setBounds (650, 210, 100, labelHeight);
    thresholdSlider_R.setBounds (630, 230, bigKnob + 40, bigKnob + 40);

    labelAttack_R.setBounds (880, 210, 80, labelHeight);
    attackSlider_R.setBounds (880, 230, 80, smallKnob + 20);
    
    labelRelease_R.setBounds (880, 310, 80, labelHeight);
    releaseSlider_R.setBounds (880, 330, 80, smallKnob + 20);

    labelBias_R.setBounds (rx, 340, 80, labelHeight);
    biasSlider_R.setBounds (rx, 360, 80, smallKnob + 20);
    
    labelRectifier_R.setBounds (820, 430, 150, labelHeight);
    rectifierSlider_R.setBounds (820, 450, 80, smallKnob + 20);

    labelOutput_R.setBounds (rx, 460, 80, labelHeight);
    outputGainSlider_R.setBounds (rx, 480, 80, smallKnob + 20);

    labelSidechain_R.setBounds (730, 420, 100, labelHeight);
    sidechainSlider_R.setBounds (730, 440, 80, smallKnob + 20);
    
    labelAirBass_R.setBounds (730, 520, 100, labelHeight);
    airBassSlider_R.setBounds (730, 540, 80, smallKnob + 20);

    // Мастер-секция
    int mx = 350;
    int mkW = 400 / 4;

    linkButton.setBounds (mx + 50, 580, 130, 24);
    midSideButton.setBounds (mx + 220, 580, 130, 24);

    labelMix.setBounds (mx, 620, mkW, labelHeight);
    mixSlider.setBounds (mx, 640, mkW, smallKnob + 20);

    labelWidth.setBounds (mx + mkW, 620, mkW, labelHeight);
    widthSlider.setBounds (mx + mkW, 640, mkW, smallKnob + 20);

    labelMono.setBounds (mx + mkW*2, 620, mkW, labelHeight);
    monoSlider.setBounds (mx + mkW*2, 640, mkW, smallKnob + 20);

    labelHeadroom.setBounds (mx + mkW*3, 620, mkW, labelHeight);
    headroomSlider.setBounds (mx + mkW*3, 640, mkW, smallKnob + 20);
}
