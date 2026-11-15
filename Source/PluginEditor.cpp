#include "PluginEditor.h"

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ironLnf);
    startTimerHz(30); // Запускаем таймер для обновления окошка GR 30 раз в секунду

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupSlider(inputGainSlider, inputLabel, "INPUT");
    setupSlider(outputGainSlider, outputLabel, "OUTPUT");
    setupSlider(thresholdSlider, thresholdLabel, "THRESHOLD");
    setupSlider(mixSlider, mixLabel, "MIX");
    setupSlider(widthSlider, widthLabel, "WIDTH");
    setupSlider(monoSlider, monoLabel, "MONO");

    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& name, const juce::StringArray& items) {
        c.addItemList (items, 1);
        addAndMakeVisible (c);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupCombo(biasLBox, biasLLabel, "BIAS L", { "Low", "Mid", "High" });
    setupCombo(biasRBox, biasRLabel, "BIAS R", { "Low", "Mid", "High" });
    setupCombo(rectifierBox, rectifierLabel, "RECTIFIER", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox, attackLabel, "ATTACK", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox, releaseLabel, "RELEASE", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });
    setupCombo(sidechainBox, sidechainLabel, "SC EQ", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" });
    setupCombo(airBassBox, airBassLabel, "AIR / TAPE", { "Bypass", "AirBass", "Tape Roll-Off" });

    // Настройка окошка Gain Reduction
    grDisplay.setText("0.0 dB", juce::dontSendNotification);
    grDisplay.setJustificationType(juce::Justification::centred);
    grDisplay.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    grDisplay.setColour(juce::Label::backgroundColourId, juce::Colour(0xff111111));
    grDisplay.setColour(juce::Label::textColourId, juce::Colour(0xffe65100));
    addAndMakeVisible(grDisplay);

    // Привязка параметров
    inputAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "inputGain", inputGainSlider);
    outputAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "outputGain", outputGainSlider);
    thresholdAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "threshold", thresholdSlider);
    mixAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "mix", mixSlider);
    widthAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "stereoWidth", widthSlider);
    monoAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "monoMaker", monoSlider);

    biasLAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "biasL", biasLBox);
    biasRAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "biasR", biasRBox);
    rectifierAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "rectifier", rectifierBox);
    attackAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "attack", attackBox);
    releaseAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "release", releaseBox);
    sidechainAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "sidechainEq", sidechainBox);
    airBassAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "airBass", airBassBox);

    setSize (800, 520); // Немного увеличили окно
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void IronCompressorAudioProcessorEditor::timerCallback()
{
    // Считываем уровень сжатия из аудио-потока
    float gr = audioProcessor.currentGRL.load();
    
    // Переводим в проценты для отображения (0-1 -> 0-100%)
    // Можно выводить в дБ, если хотите.
    if (gr < 0.01f) grDisplay.setText("0.0 %", juce::dontSendNotification);
    else grDisplay.setText(juce::String(gr * 100.0f, 1) + " %", juce::dontSendNotification);
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
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50);

    const int knobSize = 80;
    int sectionWidth = getWidth() / 6; // Теперь 6 колонок

    // --- Ряд 1: Слайдеры ---
    auto row1 = area.removeFromTop (130);
    auto layoutKnob = [&](juce::Slider& s, juce::Label& l) {
        auto itemArea = row1.removeFromLeft (sectionWidth);
        l.setBounds (itemArea.removeFromTop (20));
        s.setBounds (itemArea.withSizeKeepingCentre (knobSize, knobSize + 20));
    };

    layoutKnob(inputGainSlider, inputLabel);
    layoutKnob(outputGainSlider, outputLabel);
    layoutKnob(thresholdSlider, thresholdLabel);
    layoutKnob(mixSlider, mixLabel);
    layoutKnob(widthSlider, widthLabel);
    layoutKnob(monoSlider, monoLabel);

    area.removeFromTop (10);

    // --- Ряд 2: Комбобоксы ---
    auto row2 = area.removeFromTop (100);
    int comboWidth = getWidth() / 7;
    auto layoutCombo = [&](juce::ComboBox& c, juce::Label& l) {
        auto itemArea = row2.removeFromLeft (comboWidth);
        itemArea.reduce (5, 0);
        l.setBounds (itemArea.removeFromTop (20));
        c.setBounds (itemArea.removeFromTop (25));
    };

    layoutCombo(biasLBox, biasLLabel);
    layoutCombo(biasRBox, biasRLabel);
    layoutCombo(rectifierBox, rectifierLabel);
    layoutCombo(attackBox, attackLabel);
    layoutCombo(releaseBox, releaseLabel);
    layoutCombo(sidechainBox, sidechainLabel);
    layoutCombo(airBassBox, airBassLabel);

    // --- Ряд 3: Окошко GR ---
    area.removeFromTop (20);
    auto row3 = area.removeFromTop (60);
    auto grArea = row3.withSizeKeepingCentre (200, 50);
    
    // Рисуем рамку вокруг окошка
    juce::Rectangle<int> labelBounds = grArea;
    g.drawRoundedRectangle (labelBounds.toFloat(), 5.0f, 2.0f); // Не забываем про paint
    grDisplay.setBounds (labelBounds);
}
