#include "PluginEditor.h"

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ironLnf);
    startTimerHz(30);

    addAndMakeVisible(vuMeter_L);
    addAndMakeVisible(vuMeter_R);

    // Настройка пресетов
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

    // Настройка TMT
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

    // Вспомогательные функции
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& name, const juce::StringArray& items) {
        c.addItemList (items, 1);
        addAndMakeVisible (c);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    // Левый канал
    setupSlider(thresholdSlider_L, labelThreshold_L, "THRESHOLD L");
    setupSlider(inputGainSlider_L, labelInput_L, "INPUT L");
    setupSlider(outputGainSlider_L, labelOutput_L, "OUTPUT L");
    setupCombo(biasBox_L, labelBias_L, "BIAS L", { "Low", "Mid", "High" });
    setupCombo(rectifierBox_L, labelRectifier_L, "RECTIFIER L", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox_L, labelAttack_L, "ATTACK L", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox_L, labelRelease_L, "RELEASE L", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });
    setupCombo(airBassBox_L, labelAirBass_L, "AIR/TAPE L", { "Bypass", "AirBass", "Tape Roll-Off" });
    setupCombo(sidechainBox_L, labelSidechain_L, "SC EQ L", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" });

    // Правый канал
    setupSlider(thresholdSlider_R, labelThreshold_R, "THRESHOLD R");
    setupSlider(inputGainSlider_R, labelInput_R, "INPUT R");
    setupSlider(outputGainSlider_R, labelOutput_R, "OUTPUT R");
    setupCombo(biasBox_R, labelBias_R, "BIAS R", { "Low", "Mid", "High" });
    setupCombo(rectifierBox_R, labelRectifier_R, "RECTIFIER R", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox_R, labelAttack_R, "ATTACK R", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox_R, labelRelease_R, "RELEASE R", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });
    setupCombo(airBassBox_R, labelAirBass_R, "AIR/TAPE R", { "Bypass", "AirBass", "Tape Roll-Off" });
    setupCombo(sidechainBox_R, labelSidechain_R, "SC EQ R", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" });

    // Мастер
    setupSlider(mixSlider, labelMix, "MIX");
    setupSlider(widthSlider, labelWidth, "WIDTH");
    setupSlider(monoSlider, labelMono, "MONO");
    setupSlider(headroomSlider, labelHeadroom, "HEADROOM");

    addAndMakeVisible(linkButton);
    addAndMakeVisible(midSideButton);

    // Attachments
    attThreshold_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdL", thresholdSlider_L);
    attInput_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainL", inputGainSlider_L);
    attOutput_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGainL", outputGainSlider_L);
    attBias_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "biasL", biasBox_L);
    attRectifier_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "rectifierL", rectifierBox_L);
    attAttack_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "attackL", attackBox_L);
    attRelease_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "releaseL", releaseBox_L);
    attAirBass_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "airBassL", airBassBox_L);
    attSidechain_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "sidechainEqL", sidechainBox_L);

    attThreshold_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdR", thresholdSlider_R);
    attInput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainR", inputGainSlider_R);
    attOutput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGainR", outputGainSlider_R);
    attBias_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "biasR", biasBox_R);
    attRectifier_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "rectifierR", rectifierBox_R);
    attAttack_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "attackR", attackBox_R);
    attRelease_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "releaseR", releaseBox_R);
    attAirBass_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "airBassR", airBassBox_R);
    attSidechain_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "sidechainEqR", sidechainBox_R);

    attMix = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    attWidth = std::make_unique<SliderAttachment>(audioProcessor.apvts, "stereoWidth", widthSlider);
    attMono = std::make_unique<SliderAttachment>(audioProcessor.apvts, "monoMaker", monoSlider);
    attHeadroom = std::make_unique<SliderAttachment>(audioProcessor.apvts, "headroom", headroomSlider);
    attLink = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "link", linkButton);
    attMidSide = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "midSide", midSideButton);

    // ИСПРАВЛЕНИЕ LINK: Копируем параметры L -> R ТОЛЬКО при отключении кнопки
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

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void IronCompressorAudioProcessorEditor::timerCallback()
{
    vuMeter_L.setLevel(audioProcessor.currentGR_dB_L.load());
    vuMeter_R.setLevel(audioProcessor.currentGR_dB_R.load());
    vuMeter_L.tick();
    vuMeter_R.tick();

    bool isTMTEnabled = audioProcessor.apvts.getRawParameterValue("tmtEnable")->load() > 0.5f;
    bool isLink = audioProcessor.apvts.getRawParameterValue("link")->load() > 0.5f;
    
    // Блокируем контролы
    thresholdSlider_R.setEnabled(!isLink);
    inputGainSlider_R.setEnabled(!isLink);
    outputGainSlider_R.setEnabled(!isLink);
    biasBox_R.setEnabled(!isLink);
    rectifierBox_R.setEnabled(!isLink);
    attackBox_R.setEnabled(!isLink);
    releaseBox_R.setEnabled(!isLink);
    airBassBox_R.setEnabled(!isLink);
    sidechainBox_R.setEnabled(!isLink);

    // Визуальная синхронизация без вмешательства в звук
    if (isLink) {
        thresholdSlider_R.setValue(thresholdSlider_L.getValue(), juce::dontSendNotification);
        inputGainSlider_R.setValue(inputGainSlider_L.getValue(), juce::dontSendNotification);
        outputGainSlider_R.setValue(outputGainSlider_L.getValue(), juce::dontSendNotification);
        biasBox_R.setSelectedItemIndex(biasBox_L.getSelectedItemIndex(), juce::dontSendNotification);
        rectifierBox_R.setSelectedItemIndex(rectifierBox_L.getSelectedItemIndex(), juce::dontSendNotification);
        attackBox_R.setSelectedItemIndex(attackBox_L.getSelectedItemIndex(), juce::dontSendNotification);
        releaseBox_R.setSelectedItemIndex(releaseBox_L.getSelectedItemIndex(), juce::dontSendNotification);
        airBassBox_R.setSelectedItemIndex(airBassBox_L.getSelectedItemIndex(), juce::dontSendNotification);
        sidechainBox_R.setSelectedItemIndex(sidechainBox_L.getSelectedItemIndex(), juce::dontSendNotification);
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
    // 1. Загружаем текстуру
    static juce::Image bgTexture = juce::ImageFileFormat::loadFrom(
        BinaryData::bg_texture_jpg, 
        BinaryData::bg_texture_jpgSize
    );

    // 2. Рисуем фон
    if (bgTexture.isValid())
    {
        g.drawImage(bgTexture, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination);
        
        // Красим в красный (сделали чуть ярче, чтобы текстура была видна)
        g.setColour(juce::Colour(0xffb30000)); 
        g.setBlendMode(juce::BlendMode::multiply); 
        g.fillAll();
        
        g.setBlendMode(juce::BlendMode::normal); 
    }
    else
    {
        // ЕСЛИ ВЫ ВИДИТЕ ОРАНЖЕВЫЙ ФОН — ЗНАЧИТ КАРТИНКА НЕ ЗАГРУЗИЛАСЬ!
        g.fillAll (juce::Colours::orange);
    }

    // 3. Верхняя панель и заголовок
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (0, 0, getWidth(), 60);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("SPL IRON", 20, 10, 200, 40, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffaaaaaa));
    g.setFont (14.0f);
    g.drawText ("Parallel Dual-Tube Compressor", 160, 15, 300, 30, juce::Justification::centredLeft);
}

void IronCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (60);

    presetBox.setBounds (getWidth() - 170, 15, 150, 30);

    const int knobSize = 65;
    const int comboHeight = 22;
    const int labelHeight = 16;
    const int colW = 150;

    // Левый канал
    int lx = 20;
    vuMeter_L.setBounds (lx + 10, 70, 280, 80);
    
    labelInput_L.setBounds (lx, 170, colW, labelHeight);
    inputGainSlider_L.setBounds (lx, 190, colW, knobSize + 20);
    labelThreshold_L.setBounds (lx + 160, 170, colW, labelHeight);
    thresholdSlider_L.setBounds (lx + 160, 190, colW, knobSize + 20);

    labelAttack_L.setBounds (lx, 290, colW, labelHeight);
    attackBox_L.setBounds (lx, 310, colW, comboHeight);
    labelRelease_L.setBounds (lx + 160, 290, colW, labelHeight);
    releaseBox_L.setBounds (lx + 160, 310, colW, comboHeight);

    labelBias_L.setBounds (lx, 360, colW, labelHeight);
    biasBox_L.setBounds (lx, 380, colW, comboHeight);
    labelRectifier_L.setBounds (lx + 160, 360, colW, labelHeight);
    rectifierBox_L.setBounds (lx + 160, 380, colW, comboHeight);

    labelOutput_L.setBounds (lx, 460, colW, labelHeight);
    outputGainSlider_L.setBounds (lx, 480, colW, knobSize + 20);
    labelAirBass_L.setBounds (lx + 160, 460, colW, labelHeight);
    airBassBox_L.setBounds (lx + 160, 480, colW, comboHeight);
    labelSidechain_L.setBounds (lx + 160, 520, colW, labelHeight);
    sidechainBox_L.setBounds (lx + 160, 540, colW, comboHeight);

    // Правый канал
    int rx = 750;
    vuMeter_R.setBounds (rx + 10, 70, 280, 80);
    
    labelInput_R.setBounds (rx, 170, colW, labelHeight);
    inputGainSlider_R.setBounds (rx, 190, colW, knobSize + 20);
    labelThreshold_R.setBounds (rx + 160, 170, colW, labelHeight);
    thresholdSlider_R.setBounds (rx + 160, 190, colW, knobSize + 20);

    labelAttack_R.setBounds (rx, 290, colW, labelHeight);
    attackBox_R.setBounds (rx, 310, colW, comboHeight);
    labelRelease_R.setBounds (rx + 160, 290, colW, labelHeight);
    releaseBox_R.setBounds (rx + 160, 310, colW, comboHeight);

    labelBias_R.setBounds (rx, 360, colW, labelHeight);
    biasBox_R.setBounds (rx, 380, colW, comboHeight);
    labelRectifier_R.setBounds (rx + 160, 360, colW, labelHeight);
    rectifierBox_R.setBounds (rx + 160, 380, colW, comboHeight);

    labelOutput_R.setBounds (rx, 460, colW, labelHeight);
    outputGainSlider_R.setBounds (rx, 480, colW, knobSize + 20);
    labelAirBass_R.setBounds (rx + 160, 460, colW, labelHeight);
    airBassBox_R.setBounds (rx + 160, 480, colW, comboHeight);
    labelSidechain_R.setBounds (rx + 160, 520, colW, labelHeight);
    sidechainBox_R.setBounds (rx + 160, 540, colW, comboHeight);

    // Центральная панель
    int cx = 400;
    tmtEnableToggle.setBounds(cx + 10, 70, 80, 24);
    tmtModeButton.setBounds (cx + 90, 70, 40, 24);
    tmtLeftBtn.setBounds (cx + 135, 70, 30, 24);
    tmtDisplay.setBounds (cx + 170, 70, 80, 24);
    tmtRightBtn.setBounds (cx + 255, 70, 30, 24);
    tmtRandomBtn.setBounds (cx + 290, 70, 40, 24);

    linkButton.setBounds (cx + 10, 120, 130, 30);
    midSideButton.setBounds (cx + 160, 120, 130, 30);

    int mkW = 300 / 4;
    labelMix.setBounds (cx, 180, mkW, labelHeight);
    mixSlider.setBounds (cx, 200, mkW, knobSize + 20);

    labelWidth.setBounds (cx + mkW, 180, mkW, labelHeight);
    widthSlider.setBounds (cx + mkW, 200, mkW, knobSize + 20);

    labelMono.setBounds (cx + mkW*2, 180, mkW, labelHeight);
    monoSlider.setBounds (cx + mkW*2, 200, mkW, knobSize + 20);

    labelHeadroom.setBounds (cx + mkW*3, 180, mkW, labelHeight);
    headroomSlider.setBounds (cx + mkW*3, 200, mkW, knobSize + 20);
}
