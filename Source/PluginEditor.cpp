#include "PluginEditor.h"

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ironLnf);

    // --- Настройка слайдеров ---
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupSlider(inputGainSlider, inputLabel, "INPUT GAIN");
    setupSlider(outputGainSlider, outputLabel, "OUTPUT GAIN");
    setupSlider(thresholdSlider, thresholdLabel, "THRESHOLD");
    setupSlider(mixSlider, mixLabel, "PARALLEL MIX");

    // --- Настройка комбобоксов ---
    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& name, const juce::StringArray& items) {
        c.addItemList (items, 1);
        addAndMakeVisible (c);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupCombo(biasBox, biasLabel, "TUBE BIAS", { "Low", "Mid", "High" });
    setupCombo(rectifierBox, rectifierLabel, "RECTIFIER", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germanium" });
    setupCombo(attackBox, attackLabel, "ATTACK", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox, releaseLabel, "RELEASE", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });
    setupCombo(sidechainBox, sidechainLabel, "SIDECHAIN EQ", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4", "Ext" });

    // --- Привязка параметров ---
    inputAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "inputGain", inputGainSlider);
    outputAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "outputGain", outputGainSlider);
    thresholdAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "threshold", thresholdSlider);
    mixAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, "mix", mixSlider);

    biasAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "bias", biasBox);
    rectifierAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "rectifier", rectifierBox);
    attackAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "attack", attackBox);
    releaseAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "release", releaseBox);
    sidechainAtt = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, "sidechainEq", sidechainBox);

    setSize (750, 480);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void IronCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Фирменный фон SPL IRON (Глубокий красный)
    g.fillAll (juce::Colour (0xff7a0000));

    // Верхняя панель
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (0, 0, getWidth(), 60);

    // Заголовок
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("SPL IRON", 20, 10, 200, 40, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffaaaaaa));
    g.setFont (14.0f);
    g.drawText ("Parallel Dual-Tube Compressor", 160, 15, 300, 30, juce::Justification::centredLeft);

    // Разделительная линия
    g.setColour (juce::Colour (0xff550000));
    g.drawLine (0, 60, getWidth(), 60, 2.0f);

    // Блоки (рисуем рамки для секций)
    g.setColour (juce::Colour (0x33ffffff));
    g.drawRoundedRectangle (15, 75, getWidth() - 30, 150, 10.0f, 1.5f); // Левый блок (Gain/Threshold)
    g.drawRoundedRectangle (15, 240, getWidth() - 30, 150, 10.0f, 1.5f); // Правый блок (Время/Диоды)
}

void IronCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (50); // Пропускаем место под заголовок

    const int knobSize = 90;
    const int boxHeight = 25;

    // --- Ряд 1: Input, Output, Threshold, Mix ---
    auto row1 = area.removeFromTop (150);
    
    auto layoutKnob = [&](juce::Slider& s, juce::Label& l, int width) {
        auto itemArea = row1.removeFromLeft (width);
        l.setBounds (itemArea.removeFromTop (20));
        s.setBounds (itemArea.withSizeKeepingCentre (knobSize, knobSize + 20));
    };

    int sectionWidth = getWidth() / 4;
    layoutKnob(inputGainSlider, inputLabel, sectionWidth);
    layoutKnob(outputGainSlider, outputLabel, sectionWidth);
    layoutKnob(thresholdSlider, thresholdLabel, sectionWidth);
    layoutKnob(mixSlider, mixLabel, sectionWidth);

    area.removeFromTop (20);

    // --- Ряд 2: Bias, Rectifier, Attack, Release, Sidechain ---
    auto row2 = area.removeFromTop (120);
    
    int comboWidth = getWidth() / 5;
    
    auto layoutCombo = [&](juce::ComboBox& c, juce::Label& l, int width) {
        auto itemArea = row2.removeFromLeft (width);
        itemArea.reduce (10, 0);
        l.setBounds (itemArea.removeFromTop (20));
        c.setBounds (itemArea.removeFromTop (boxHeight));
    };

    layoutCombo(biasBox, biasLabel, comboWidth);
    layoutCombo(rectifierBox, rectifierLabel, comboWidth);
    layoutCombo(attackBox, attackLabel, comboWidth);
    layoutCombo(releaseBox, releaseLabel, comboWidth);
    layoutCombo(sidechainBox, sidechainLabel, comboWidth);
}
