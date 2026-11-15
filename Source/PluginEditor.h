#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ... (Классы AnalogVUMeter и IronLookAndFeel остаются без изменений, я их пропущу для краткости) ...

class IronCompressorAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    explicit IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor&);
    ~IronCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    IronCompressorAudioProcessor& audioProcessor;
    IronLookAndFeel ironLnf;

    AnalogVUMeter vuMeter_L {"GR L"};
    AnalogVUMeter vuMeter_R {"GR R"};

    // Пресеты
    juce::ComboBox presetBox;

    // TMT
    juce::ToggleButton tmtEnableToggle { "TMT ON" }; 
    juce::TextButton tmtModeButton{"A"};
    juce::TextButton tmtLeftBtn{"<"};
    juce::TextButton tmtRightBtn{">"};
    juce::TextButton tmtRandomBtn{"RND"};
    juce::Label tmtDisplay;

    // ... (Все остальные слайдеры и лейблы остаются как были) ...
    juce::Slider thresholdSlider_L, inputGainSlider_L, outputGainSlider_L;
    juce::ComboBox biasBox_L, rectifierBox_L, attackBox_L, releaseBox_L, airBassBox_L, sidechainBox_L;
    juce::Label labelThreshold_L, labelInput_L, labelOutput_L, labelBias_L, labelRectifier_L, labelAttack_L, labelRelease_L, labelAirBass_L, labelSidechain_L;

    juce::Slider thresholdSlider_R, inputGainSlider_R, outputGainSlider_R;
    juce::ComboBox biasBox_R, rectifierBox_R, attackBox_R, releaseBox_R, airBassBox_R, sidechainBox_R;
    juce::Label labelThreshold_R, labelInput_R, labelOutput_R, labelBias_R, labelRectifier_R, labelAttack_R, labelRelease_R, labelAirBass_R, labelSidechain_R;

    juce::Slider mixSlider, widthSlider, monoSlider, headroomSlider;
    juce::Label labelMix, labelWidth, labelMono, labelHeadroom;

    juce::ToggleButton linkButton { "Link L/R" };
    juce::ToggleButton midSideButton { "M/S Mode" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> attThreshold_L, attInput_L, attOutput_L, attThreshold_R, attInput_R, attOutput_R;
    std::unique_ptr<SliderAttachment> attMix, attWidth, attMono, attHeadroom;
    std::unique_ptr<ComboBoxAttachment> attBias_L, attRectifier_L, attAttack_L, attRelease_L, attAirBass_L, attSidechain_L;
    std::unique_ptr<ComboBoxAttachment> attBias_R, attRectifier_R, attAttack_R, attRelease_R, attAirBass_R, attSidechain_R;
    std::unique_ptr<ButtonAttachment> attLink, attMidSide, attTMTMode, attTMTEnable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};
