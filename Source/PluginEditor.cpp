#include "PluginEditor.h"

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ironLnf);
    startTimerHz(30);

    addAndMakeVisible(vuMeter_L);
    addAndMakeVisible(vuMeter_R);

    // --- Настройка TMT UI ---
    addAndMakeVisible(tmtEnableToggle);
    addAndMakeVisible(tmtModeButton);
    addAndMakeVisible(tmtLeftBtn);
    addAndMakeVisible(tmtRightBtn);
    addAndMakeVisible(tmtRandomBtn);
    addAndMakeVisible(tmtDisplay);
    addAndMakeVisible(tmtTitleLabel);

    tmtTitleLabel.setText("TMT", juce::dontSendNotification);
    tmtTitleLabel.setJustificationType(juce::Justification::centred);
    tmtTitleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    
    tmtDisplay.setJustificationType(juce::Justification::centred);
    tmtDisplay.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    tmtDisplay.setColour(juce::Label::backgroundColourId, juce::Colour(0xff111111));
    tmtDisplay.setColour(juce::Label::textColourId, juce::Colour(0xffcc0000));
    
    attTMTMode = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "tmtStereoMode", tmtModeButton);
    attTMTEnable = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "tmtEnable", tmtEnableToggle);

    tmtModeButton.onClick = [this]() {
        bool isDigital = audioProcessor.apvts.getRawParameterValue("tmtStereoMode")->load() > 0.5f;
        tmtModeButton.setButtonText(isDigital ? "A" : "D");
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

    // --- Остальные контролы ---
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

    setupSlider(thresholdSlider_L, labelThreshold_L, "THRESHOLD L");
    setupSlider(inputGainSlider_L, labelInput_L, "INPUT L");
    setupCombo(biasBox_L, labelBias_L, "BIAS L", { "Low", "Mid", "High" });
    setupCombo(rectifierBox_L, labelRectifier_L, "RECTIFIER L", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox_L, labelAttack_L, "ATTACK L", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox_L, labelRelease_L, "RELEASE L", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });

    setupSlider(thresholdSlider_R, labelThreshold_R, "THRESHOLD R");
    setupSlider(inputGainSlider_R, labelInput_R, "INPUT R");
    setupCombo(biasBox_R, labelBias_R, "BIAS R", { "Low", "Mid", "High" });
    setupCombo(rectifierBox_R, labelRectifier_R, "RECTIFIER R", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox_R, labelAttack_R, "ATTACK R", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox_R, labelRelease_R, "RELEASE R", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });

    setupSlider(outputGainSlider, labelOutput, "OUTPUT");
    setupSlider(mixSlider, labelMix, "MIX");
    setupSlider(widthSlider, labelWidth, "WIDTH");
    setupSlider(monoSlider, labelMono, "MONO");
    setupSlider(headroomSlider, labelHeadroom, "HEADROOM");

    setupCombo(sidechainBox, labelSidechain, "SC EQ", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" });
    setupCombo(airBassBox, labelAirBass, "AIR / TAPE", { "Bypass", "AirBass", "Tape Roll-Off" });

    addAndMakeVisible(linkButton);
    addAndMakeVisible(midSideButton);

    attThreshold_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdL", thresholdSlider_L);
    attInput_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainL", inputGainSlider_L);
    attBias_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "biasL", biasBox_L);
    attRectifier_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "rectifierL", rectifierBox_L);
    attAttack_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "attackL", attackBox_L);
    attRelease_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "releaseL", releaseBox_L);

    attThreshold_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdR", thresholdSlider_R);
    attInput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainR", inputGainSlider_R);
    attBias_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "biasR", biasBox_R);
    attRectifier_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "rectifierR", rectifierBox_R);
    attAttack_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "attackR", attackBox_R);
    attRelease_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "releaseR", releaseBox_R);

    attOutput = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGain", outputGainSlider);
    attMix = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    attWidth = std::make_unique<SliderAttachment>(audioProcessor.apvts, "stereoWidth", widthSlider);
    attMono = std::make_unique<SliderAttachment>(audioProcessor.apvts, "monoMaker", monoSlider);
    attHeadroom = std::make_unique<SliderAttachment>(audioProcessor.apvts, "headroom", headroomSlider);
    attSidechain = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "sidechainEq", sidechainBox);
    attAirBass = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "airBass", airBassBox);

    attLink = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "link", linkButton);
    attMidSide = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "midSide", midSideButton);

    setSize (1050, 700);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void IronCompressorAudioProcessorEditor::timerCallback()
{
    vuMeter_L.setLevel(audioProcessor.currentGR_dB_L.load());
    vuMeter_R.setLevel(audioProcessor.currentGR_dB_R.load());
    
    vuMeter_L.tick();
    vuMeter_R.tick();

    // Проверяем, включен ли TMT
    bool isTMTEnabled = audioProcessor.apvts.getRawParameterValue("tmtEnable")->load() > 0.5f;
    
    // Включаем/выключаем доступность элементов UI
    tmtModeButton.setEnabled(isTMTEnabled);
    tmtLeftBtn.setEnabled(isTMTEnabled);
    tmtRightBtn.setEnabled(isTMTEnabled);
    tmtRandomBtn.setEnabled(isTMTEnabled);
    tmtDisplay.setAlpha(isTMTEnabled ? 1.0f : 0.4f); // Делаем дисплей полупрозрачным, если выключено

    // Обновляем дисплей TMT, только если он активен
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
    g.fillAll (juce::Colour (0xff7a0000));
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

    const int knobSize = 75;
    const int comboHeight = 25;
    const int labelHeight = 18;
    const int colWidth = 250;

    // --- ЛЕВЫЙ КАНАЛ (X: 20) ---
    int leftX = 20;
    vuMeter_L.setBounds (leftX + 40, 70, 170, 70);
    
    labelInput_L.setBounds (leftX, 150, colWidth/2, labelHeight);
    inputGainSlider_L.setBounds (leftX, 170, colWidth/2, knobSize + 20);
    
    labelThreshold_L.setBounds (leftX + colWidth/2, 150, colWidth/2, labelHeight);
    thresholdSlider_L.setBounds (leftX + colWidth/2, 170, colWidth/2, knobSize + 20);

    labelAttack_L.setBounds (leftX, 280, colWidth/2, labelHeight);
    attackBox_L.setBounds (leftX, 300, colWidth/2 - 5, comboHeight);
    
    labelRelease_L.setBounds (leftX + colWidth/2, 280, colWidth/2, labelHeight);
    releaseBox_L.setBounds (leftX + colWidth/2, 300, colWidth/2 - 5, comboHeight);

    labelBias_L.setBounds (leftX, 360, colWidth/2, labelHeight);
    biasBox_L.setBounds (leftX, 380, colWidth/2 - 5, comboHeight);
    
    labelRectifier_L.setBounds (leftX + colWidth/2, 360, colWidth/2, labelHeight);
    rectifierBox_L.setBounds (leftX + colWidth/2, 380, colWidth/2 - 5, comboHeight);

    // --- ПРАВЫЙ КАНАЛ (X: 320) ---
    int rightX = 320;
    vuMeter_R.setBounds (rightX + 40, 70, 170, 70);
    
    labelInput_R.setBounds (rightX, 150, colWidth/2, labelHeight);
    inputGainSlider_R.setBounds (rightX, 170, colWidth/2, knobSize + 20);
    
    labelThreshold_R.setBounds (rightX + colWidth/2, 150, colWidth/2, labelHeight);
    thresholdSlider_R.setBounds (rightX + colWidth/2, 170, colWidth/2, knobSize + 20);

    labelAttack_R.setBounds (rightX, 280, colWidth/2, labelHeight);
    attackBox_R.setBounds (rightX, 300, colWidth/2 - 5, comboHeight);
    
    labelRelease_R.setBounds (rightX + colWidth/2, 280, colWidth/2, labelHeight);
    releaseBox_R.setBounds (rightX + colWidth/2, 300, colWidth/2 - 5, comboHeight);

    labelBias_R.setBounds (rightX, 360, colWidth/2, labelHeight);
    biasBox_R.setBounds (rightX, 380, colWidth/2 - 5, comboHeight);
    
    labelRectifier_R.setBounds (rightX + colWidth/2, 360, colWidth/2, labelHeight);
    rectifierBox_R.setBounds (rightX + colWidth/2, 380, colWidth/2 - 5, comboHeight);

    // --- ЦЕНТРАЛЬНАЯ СЕКЦИЯ (X: 620) ---
    int centerX = 620;
    
    // Секция TMT
    auto tmtArea = juce::Rectangle<int>(centerX + 20, 70, 390, 70);
    
    // Верхняя строка: Заголовок TMT и кнопка ВКЛ/ВЫКЛ
    auto topRow = tmtArea.removeFromTop(20);
    tmtTitleLabel.setBounds(topRow.removeFromLeft(50));
    tmtEnableToggle.setBounds(topRow.removeFromLeft(50));
    
    auto btnArea = tmtArea;
    tmtModeButton.setBounds (btnArea.removeFromLeft(50).reduced(2));
    tmtLeftBtn.setBounds (btnArea.removeFromLeft(40).reduced(2));
    tmtDisplay.setBounds (btnArea.removeFromLeft(120).reduced(2));
    tmtRightBtn.setBounds (btnArea.removeFromLeft(40).reduced(2));
    tmtRandomBtn.setBounds (btnArea.removeFromLeft(60).reduced(2));

    // Ручки мастера (в 2 ряда)
    int masterKnobW = 390 / 3;
    
    labelOutput.setBounds (centerX + 20, 160, masterKnobW, labelHeight);
    outputGainSlider.setBounds (centerX + 20, 180, masterKnobW, knobSize + 20);
    
    labelMix.setBounds (centerX + 20 + masterKnobW, 160, masterKnobW, labelHeight);
    mixSlider.setBounds (centerX + 20 + masterKnobW, 180, masterKnobW, knobSize + 20);
    
    labelHeadroom.setBounds (centerX + 20 + masterKnobW*2, 160, masterKnobW, labelHeight);
    headroomSlider.setBounds (centerX + 20 + masterKnobW*2, 180, masterKnobW, knobSize + 20);

    labelWidth.setBounds (centerX + 20 + masterKnobW/2, 290, masterKnobW, labelHeight);
    widthSlider.setBounds (centerX + 20 + masterKnobW/2, 310, masterKnobW, knobSize + 20);
    
    labelMono.setBounds (centerX + 20 + masterKnobW*2 - 30, 290, masterKnobW, labelHeight);
    monoSlider.setBounds (centerX + 20 + masterKnobW*2 - 30, 310, masterKnobW, knobSize + 20);

    // Нижние комбобоксы
    labelSidechain.setBounds (centerX + 20, 430, 180, labelHeight);
    sidechainBox.setBounds (centerX + 20, 450, 180, comboHeight);

    labelAirBass.setBounds (centerX + 210, 430, 180, labelHeight);
    airBassBox.setBounds (centerX + 210, 450, 180, comboHeight);

    // Переключатели Link и M/S
    linkButton.setBounds (centerX + 20, 500, 180, 30);
    midSideButton.setBounds (centerX + 210, 500, 180, 30);
}
