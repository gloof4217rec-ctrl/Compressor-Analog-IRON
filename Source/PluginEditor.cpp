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
            setC("attackL", 2); setC("attackR", 2); setC("releaseL", 2); setC("releaseR", 2);
            setC("biasL", 1); setC("biasR", 1);
            setC("sidechainEqL", 1); setC("sidechainEqR", 1);
            setP("stereoWidth", 1.2f); setP("mix", 1.0f);
        } else if (id == 3) {
            setP("thresholdL", -10.0f); setP("thresholdR", -10.0f);
            setC("attackL", 0); setC("attackR", 0); setC("releaseL", 1); setC("releaseR", 1);
            setC("biasL", 2); setC("biasR", 2);
            setC("sidechainEqL", 0); setC("sidechainEqR", 0);
            setP("stereoWidth", 1.0f); setP("mix", 0.8f);
        } else if (id == 4) {
            setP("thresholdL", -20.0f); setP("thresholdR", -20.0f);
            setC("attackL", 3); setC("attackR", 3); setC("releaseL", 3); setC("releaseR", 3);
            setC("biasL", 0); setC("biasR", 0);
            setC("airBassL", 1); setC("airBassR", 1);
            setP("stereoWidth", 1.5f); setP("mix", 1.0f);
        } else {
            setP("thresholdL", -10.0f); setP("thresholdR", -10.0f);
            setC("attackL", 2); setC("attackR", 2); setC("releaseL", 2); setC("releaseR", 2);
            setC("biasL", 0); setC("biasR", 0);
            setC("sidechainEqL", 0); setC("sidechainEqR", 0);
            setC("airBassL", 0); setC("airBassR", 0);
            setP("stereoWidth", 1.0f); setP("mix", 1.0f);
        }
    };
    addAndMakeVisible(presetBox);

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

    auto setupLabel = [this](juce::Label& l, const juce::String& name) {
        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colours::white);
        l.setLookAndFeel(&shadowLnf);
        addAndMakeVisible(l);
    };
    setupLabel(labelTmtOn, "TMT");
    setupLabel(labelLink, "LINK");
    setupLabel(labelMidSide, "M/S");

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        s.setLookAndFeel(&filmstripLnf); 
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colours::white);
        l.setLookAndFeel(&shadowLnf);
        l.setMinimumHorizontalScale(0.5f);
        addAndMakeVisible (l);
    };

    auto setupSteppedSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name, int numSteps, const juce::StringArray& labels) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0); 
        s.setRange(0.0, (double)(numSteps - 1), 1.0); 
        s.setLookAndFeel(&filmstripLnf); 
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, juce::Colours::white);
        l.setLookAndFeel(&shadowLnf);
        l.setMinimumHorizontalScale(0.5f);
        addAndMakeVisible (l);
        
        if (numSteps > 1 && labels.size() == numSteps) {
            s.getProperties().set("labels", labels.joinIntoString(","));
        }
    };

    juce::StringArray attackLabels = {"0.1", "0.2", "0.5", "1", "2", "5"};
    juce::StringArray releaseLabels = {"200m", "300m", "500m", "1s", "2s", "5s"};
    juce::StringArray rectLabels = {"Ge1", "Ge2", "LED", "Si", "Ge3", "Ge4"};
    juce::StringArray scLabels = {"Off", "Pr1", "Pr2", "Pr3", "Pr4"};

    // Левый канал
    setupSlider(thresholdSlider_L, labelThreshold_L, "THRESHOLD L");
    setupSlider(inputGainSlider_L, labelInput_L, "INPUT L");
    setupSlider(outputGainSlider_L, labelOutput_L, "OUTPUT L");
    setupSteppedSlider(attackSlider_L, labelAttack_L, "ATTACK L", 6, attackLabels);
    setupSteppedSlider(releaseSlider_L, labelRelease_L, "RELEASE L", 6, releaseLabels);
    setupSteppedSlider(rectifierSlider_L, labelRectifier_L, "RECTIFIER L", 6, rectLabels);
    setupSteppedSlider(sidechainSlider_L, labelSidechain_L, "SC EQ L", 5, scLabels);

    biasSlider_L.setSliderStyle(juce::Slider::LinearVertical);
    biasSlider_L.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    biasSlider_L.setRange(0.0, 2.0, 1.0);
    biasSlider_L.setLookAndFeel(&biasLnf);
    addAndMakeVisible(biasSlider_L);
    labelBias_L.setText("BIAS L", juce::dontSendNotification);
    labelBias_L.setJustificationType(juce::Justification::centred);
    labelBias_L.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    labelBias_L.setColour(juce::Label::textColourId, juce::Colours::white);
    labelBias_L.setLookAndFeel(&shadowLnf);
    addAndMakeVisible(labelBias_L);

    airBassSlider_L.setSliderStyle(juce::Slider::LinearVertical);
    airBassSlider_L.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    airBassSlider_L.setRange(0.0, 2.0, 1.0);
    airBassSlider_L.setLookAndFeel(&switchLnf);
    addAndMakeVisible(airBassSlider_L);
    labelAirBass_L.setText("AIR/TAPE L", juce::dontSendNotification);
    labelAirBass_L.setJustificationType(juce::Justification::centred);
    labelAirBass_L.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    labelAirBass_L.setColour(juce::Label::textColourId, juce::Colours::white);
    labelAirBass_L.setLookAndFeel(&shadowLnf);
    addAndMakeVisible(labelAirBass_L);

    // Правый канал
    setupSlider(thresholdSlider_R, labelThreshold_R, "THRESHOLD R");
    setupSlider(inputGainSlider_R, labelInput_R, "INPUT R");
    setupSlider(outputGainSlider_R, labelOutput_R, "OUTPUT R");
    setupSteppedSlider(attackSlider_R, labelAttack_R, "ATTACK R", 6, attackLabels);
    setupSteppedSlider(releaseSlider_R, labelRelease_R, "RELEASE R", 6, releaseLabels);
    setupSteppedSlider(rectifierSlider_R, labelRectifier_R, "RECTIFIER R", 6, rectLabels);
    setupSteppedSlider(sidechainSlider_R, labelSidechain_R, "SC EQ R", 5, scLabels);

    biasSlider_R.setSliderStyle(juce::Slider::LinearVertical);
    biasSlider_R.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    biasSlider_R.setRange(0.0, 2.0, 1.0);
    biasSlider_R.setLookAndFeel(&biasLnf);
    addAndMakeVisible(biasSlider_R);
    labelBias_R.setText("BIAS R", juce::dontSendNotification);
    labelBias_R.setJustificationType(juce::Justification::centred);
    labelBias_R.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    labelBias_R.setColour(juce::Label::textColourId, juce::Colours::white);
    labelBias_R.setLookAndFeel(&shadowLnf);
    addAndMakeVisible(labelBias_R);

    airBassSlider_R.setSliderStyle(juce::Slider::LinearVertical);
    airBassSlider_R.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    airBassSlider_R.setRange(0.0, 2.0, 1.0);
    airBassSlider_R.setLookAndFeel(&switchLnf);
    addAndMakeVisible(airBassSlider_R);
    labelAirBass_R.setText("AIR/TAPE R", juce::dontSendNotification);
    labelAirBass_R.setJustificationType(juce::Justification::centred);
    labelAirBass_R.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    labelAirBass_R.setColour(juce::Label::textColourId, juce::Colours::white);
    labelAirBass_R.setLookAndFeel(&shadowLnf);
    addAndMakeVisible(labelAirBass_R);

    // Мастер
    setupSlider(mixSlider, labelMix, "MIX");
    setupSlider(widthSlider, labelWidth, "WIDTH");
    setupSlider(monoSlider, labelMono, "MONO");
    setupSlider(headroomSlider, labelHeadroom, "HEADROOM");

    linkButton.setLookAndFeel(&imageToggleLnf);
    midSideButton.setLookAndFeel(&imageToggleLnf);
    tmtEnableToggle.setLookAndFeel(&imageToggleLnf);

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
    attSidechain_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "sidechainEqL", sidechainSlider_L);
    attAirBass_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "airBassL", airBassSlider_L);

    attThreshold_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdR", thresholdSlider_R);
    attInput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainR", inputGainSlider_R);
    attOutput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGainR", outputGainSlider_R);
    attAttack_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attackR", attackSlider_R);
    attRelease_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "releaseR", releaseSlider_R);
    attBias_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "biasR", biasSlider_R);
    attRectifier_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "rectifierR", rectifierSlider_R);
    attSidechain_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "sidechainEqR", sidechainSlider_R);
    attAirBass_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "airBassR", airBassSlider_R);

    attMix = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    attWidth = std::make_unique<SliderAttachment>(audioProcessor.apvts, "stereoWidth", widthSlider);
    attMono = std::make_unique<SliderAttachment>(audioProcessor.apvts, "monoMaker", monoSlider);
    attHeadroom = std::make_unique<SliderAttachment>(audioProcessor.apvts, "headroom", headroomSlider);
    attLink = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "link", linkButton);
    attMidSide = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "midSide", midSideButton);

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

    setSize (1150, 750);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() 
{ 
    setLookAndFeel(nullptr); 
}

void IronCompressorAudioProcessorEditor::timerCallback()
{
    static const juce::StringArray attackVals {"0.0001 s", "0.0002 s", "0.0005 s", "0.001 s", "0.002 s", "0.005 s"};
    static const juce::StringArray releaseVals {"0.2 s", "0.3 s", "0.5 s", "1 s", "2 s", "5 s"};
    static const juce::StringArray biasVals {"Low", "Mid", "High"};
    static const juce::StringArray rectifierVals {"Ge 1mF", "Ge 2mF", "LED 3.3mF", "Si 330nF", "Ge 220nF", "Ge/Si"};
    static const juce::StringArray sidechainVals {"Off", "Pr 1", "Pr 2", "Pr 3", "Pr 4"};
    static const juce::StringArray airBassVals {"Bypass", "AirBass", "Tape"};

    int attIdxL = juce::jlimit(0, 5, (int)attackSlider_L.getValue());
    int relIdxL = juce::jlimit(0, 5, (int)releaseSlider_L.getValue());
    int biasIdxL = juce::jlimit(0, 2, (int)biasSlider_L.getValue());
    int rectIdxL = juce::jlimit(0, 5, (int)rectifierSlider_L.getValue());
    int scIdxL = juce::jlimit(0, 4, (int)sidechainSlider_L.getValue());
    int airIdxL = juce::jlimit(0, 2, (int)airBassSlider_L.getValue());

    labelAttack_L.setText("ATK L: " + attackVals[attIdxL], juce::dontSendNotification);
    labelRelease_L.setText("REL L: " + releaseVals[relIdxL], juce::dontSendNotification);
    labelBias_L.setText("BIAS L: " + biasVals[biasIdxL], juce::dontSendNotification);
    labelRectifier_L.setText("RECT L: " + rectifierVals[rectIdxL], juce::dontSendNotification);
    labelSidechain_L.setText("SC EQ L: " + sidechainVals[scIdxL], juce::dontSendNotification);
    labelAirBass_L.setText("AIR/TAPE L: " + airBassVals[airIdxL], juce::dontSendNotification);

    int attIdxR = juce::jlimit(0, 5, (int)attackSlider_R.getValue());
    int relIdxR = juce::jlimit(0, 5, (int)releaseSlider_R.getValue());
    int biasIdxR = juce::jlimit(0, 2, (int)biasSlider_R.getValue());
    int rectIdxR = juce::jlimit(0, 5, (int)rectifierSlider_R.getValue());
    int scIdxR = juce::jlimit(0, 4, (int)sidechainSlider_R.getValue());
    int airIdxR = juce::jlimit(0, 2, (int)airBassSlider_R.getValue());

    labelAttack_R.setText("ATK R: " + attackVals[attIdxR], juce::dontSendNotification);
    labelRelease_R.setText("REL R: " + releaseVals[relIdxR], juce::dontSendNotification);
    labelBias_R.setText("BIAS R: " + biasVals[biasIdxR], juce::dontSendNotification);
    labelRectifier_R.setText("RECT R: " + rectifierVals[rectIdxR], juce::dontSendNotification);
    labelSidechain_R.setText("SC EQ R: " + sidechainVals[scIdxR], juce::dontSendNotification);
    labelAirBass_R.setText("AIR/TAPE R: " + airBassVals[airIdxR], juce::dontSendNotification);

    vuMeter_L.setLevel(audioProcessor.currentGR_dB_L.load());
    vuMeter_R.setLevel(audioProcessor.currentGR_dB_R.load());
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
    g.fillRoundedRectangle(410.0f, 70.0f, 330.0f, 140.0f, 5.0f);
}

void IronCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (60);

    presetBox.setBounds (475, 15, 160, 30);

    const int knobSize = 80;
    const int bigKnobSize = 110;

    // УВЕЛИЧЕНЫ ОТСТУПЫ ДЛЯ ТЕКСТА, РУЧКИ ОПУЩЕНЫ ВНИЗ
    auto posKnob = [](juce::Label& l, juce::Slider& s, int x, int y, int w, int h) {
        l.setBounds(x - 30, y, w + 60, 20); 
        s.setBounds(x, y + 25, w, h);       
    };

    // ==========================================
    // ЦЕНТРАЛЬНАЯ СЕКЦИЯ (Метры, TMT, Мастер)
    // ==========================================
    vuMeter_L.setBounds (420, 80, 150, 110);
    vuMeter_R.setBounds (580, 80, 150, 110);

    // Threshold в центр
    posKnob(labelThreshold_L, thresholdSlider_L, 400, 220, bigKnobSize, bigKnobSize);
    posKnob(labelThreshold_R, thresholdSlider_R, 640, 220, bigKnobSize, bigKnobSize);

    // Выравниваем TMT идеально по центру
    int cx = 575; 
    labelTmtOn.setBounds(cx - 20, 380, 40, 20);
    tmtEnableToggle.setBounds (cx - 40, 400, 80, 30); 

    tmtModeButton.setBounds (cx - 110, 440, 40, 24);
    tmtLeftBtn.setBounds (cx - 65, 440, 30, 24);
    tmtDisplay.setBounds (cx - 30, 440, 60, 24);
    tmtRightBtn.setBounds (cx + 35, 440, 30, 24);
    tmtRandomBtn.setBounds (cx + 70, 440, 40, 24);

    labelLink.setBounds (460, 500, 80, 20);
    linkButton.setBounds (460, 520, 80, 24);

    labelMidSide.setBounds (620, 500, 80, 20);
    midSideButton.setBounds (620, 520, 80, 24);

    // Мастер (снизу по центру, подняли выше)
    posKnob(labelMix, mixSlider, 420, 640, knobSize, knobSize);
    posKnob(labelWidth, widthSlider, 520, 640, knobSize, knobSize);
    posKnob(labelMono, monoSlider, 620, 640, knobSize, knobSize);
    posKnob(labelHeadroom, headroomSlider, 720, 640, knobSize, knobSize);

    // ==========================================
    // ЛЕВЫЙ КАНАЛ (x: 20, 150, 280)
    // ==========================================
    int lx = 20;
    // Ряд 1
    posKnob(labelInput_L, inputGainSlider_L, lx, 100, knobSize, knobSize);
    posKnob(labelAttack_L, attackSlider_L, lx + 130, 100, knobSize, knobSize);
    posKnob(labelRelease_L, releaseSlider_L, lx + 260, 100, knobSize, knobSize);
    
    // Ряд 2
    posKnob(labelOutput_L, outputGainSlider_L, lx, 240, knobSize, knobSize);
    labelBias_L.setBounds(lx + 130, 240, 80, 20);
    biasSlider_L.setBounds(lx + 140, 270, 60, 90);
    posKnob(labelRectifier_L, rectifierSlider_L, lx + 260, 240, knobSize, knobSize);

    // Ряд 3
    posKnob(labelSidechain_L, sidechainSlider_L, lx, 400, knobSize, knobSize);
    labelAirBass_L.setBounds(lx + 130, 400, 100, 20);
    airBassSlider_L.setBounds(lx + 140, 430, 60, 90);

    // ==========================================
    // ПРАВЫЙ КАНАЛ (x: 1150 - 20 - 80 - 260 = 790)
    // ==========================================
    int rx = 1150 - 20 - knobSize; // 1050
    // Ряд 1
    posKnob(labelRelease_R, releaseSlider_R, rx, 100, knobSize, knobSize);
    posKnob(labelAttack_R, attackSlider_R, rx - 130, 100, knobSize, knobSize);
    posKnob(labelInput_R, inputGainSlider_R, rx - 260, 100, knobSize, knobSize);

    // Ряд 2
    posKnob(labelRectifier_R, rectifierSlider_R, rx, 240, knobSize, knobSize);
    labelBias_R.setBounds(rx - 130, 240, 80, 20);
    biasSlider_R.setBounds(rx - 120, 270, 60, 90);
    posKnob(labelOutput_R, outputGainSlider_R, rx - 260, 240, knobSize, knobSize);

    // Ряд 3
    labelAirBass_R.setBounds(rx - 130, 400, 100, 20);
    airBassSlider_R.setBounds(rx - 120, 430, 60, 90);
    posKnob(labelSidechain_R, sidechainSlider_R, rx - 260, 400, knobSize, knobSize);
}
