#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class IronLookAndFeel : public juce::LookAndFeel_V4 {
    // ... (Оставляем ваш код из предыдущего шага без изменений) ...
};

class IronCompressorAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor&);
    ~IronCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override; // Для обновления GR

private:
    IronCompressorAudioProcessor& audioProcessor;
    IronLookAndFeel ironLnf;

    juce::Slider inputGainSlider, outputGainSlider, thresholdSlider, mixSlider;
    juce::Slider widthSlider, monoSlider; // Новые ручки
    
    juce::ComboBox biasLBox, biasRBox, rectifierBox, attackBox, releaseBox, sidechainBox, airBassBox;
    juce::Label inputLabel, outputLabel, thresholdLabel, mixLabel, widthLabel, monoLabel;
    juce::Label biasLLabel, biasRLabel, rectifierLabel, attackLabel, releaseLabel, sidechainLabel, airBassLabel;

    // Окошко для Gain Reduction
    juce::Label grDisplay;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> inputAtt, outputAtt, thresholdAtt, mixAtt, widthAtt, monoAtt;
    std::unique_ptr<ComboBoxAttachment> biasLAtt, biasRAtt, rectifierAtt, attackAtt, releaseAtt, sidechainAtt, airBassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};
