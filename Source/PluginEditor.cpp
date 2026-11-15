#include "PluginEditor.h"

// ... (Классы AnalogVUMeter и IronLookAndFeel без изменений) ...

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

        if (id == 2) { // Vocal
            setP("thresholdL", -15.0f); setP("thresholdR", -15.0f);
            setC("attackL", 2); setC("attackR", 2); // 0.5 ms
            setC("releaseL", 2); setC("releaseR", 2); // 500 ms
            setC("biasL", 1); setC("biasR", 1); // Mid
            setC("sidechainEqL", 1); setC("sidechainEqR", 1);
            setP("stereoWidth", 1.2f);
            setP("mix", 1.0f);
        } else if (id == 3) { // Drum
            setP("thresholdL", -10.0f); setP("thresholdR", -10.0f);
            setC("attackL", 0); setC("attackR", 0); // 0.1 ms
            setC("releaseL", 1); setC("releaseR", 1); // 300 ms
            setC("biasL", 2); setC("biasR", 2); // High
            setC("sidechainEqL", 0); setC("sidechainEqR", 0);
            setP("stereoWidth", 1.0f);
            setP("mix", 0.8f); // Параллельная компрессия
        } else if (id == 4) { // Keys
            setP("thresholdL", -20.0f); setP("thresholdR", -20.0f);
            setC("attackL", 3); setC("attackR", 3); // 1 ms
            setC("releaseL", 3); setC("releaseR", 3); // 1 s
            setC("biasL", 0); setC("biasR", 0); // Low
            setC("airBassL", 1); setC("airBassR", 1); // AirBass
            setP("stereoWidth", 1.5f);
            setP("mix", 1.0f);
        } else { // Default
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

    // TMT Setup
    // ... (Код TMT без изменений, он полностью совпадает с предыдущей версией) ...

    // Helpers и Attachments
    // ... (Все остальные привязки параметров точно такие же, как в предыдущем коде) ...

    setSize (1100, 750);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void IronCompressorAudioProcessorEditor::timerCallback()
{
    // ... (Код timerCallback без изменений, он синхронизирует Link и TMT) ...
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

    // Пресеты в правом верхнем углу
    presetBox.setBounds (getWidth() - 170, 15, 150, 30);

    const int knobSize = 65;
    const int comboHeight = 22;
    const int labelHeight = 16;
    const int colW = 150;

    // ... (Весь остальной код resized без изменений) ...
    // (Он идентичен предыдущему, где мы выстроили симметричную сетку)
    // Левый, Правый, Центр...
    
    // Только для полноты картины вставлю пару строк:
    int lx = 20;
    vuMeter_L.setBounds (lx + 10, 70, 280, 80);
    // ... и т.д.
    
    // (Скопируйте блок resized из предыдущего ответа полностью)
}
