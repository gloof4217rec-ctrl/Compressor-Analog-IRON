#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class IronCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor&);
    ~IronCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    IronCompressorAudioProcessor& audioProcessor;

    juce::Slider thresholdSlider, ratioSlider, biasSlider, mixSlider,
                 inputGainSlider, outputGainSlider;
    juce::ComboBox rectifierBox, attackBox, releaseBox;
    juce::ToggleButton sidechainHPFToggle { "Sidechain HPF (20 Hz)" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> thresholdAtt, ratioAtt, biasAtt, mixAtt,
                                      inputGainAtt, outputGainAtt;
    std::unique_ptr<ComboBoxAttachment> rectifierAtt, attackAtt, releaseAtt;
    std::unique_ptr<ButtonAttachment> sidechainHPFAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};