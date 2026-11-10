#include "PluginEditor.h"

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (
        IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // ---- Threshold ----
    thresholdSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    thresholdSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    thresholdSlider.setColour (juce::Slider::rotarySliderFillColourId,
                               juce::Colours::orange);
    addAndMakeVisible (thresholdSlider);
    thresholdAtt = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, "threshold", thresholdSlider);

    // ---- Ratio ----
    ratioSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    ratioSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (ratioSlider);
    ratioAtt = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, "ratio", ratioSlider);

    // ---- Bias ----
    biasSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    biasSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (biasSlider);
    biasAtt = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, "bias", biasSlider);

    // ---- Mix ----
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (mixSlider);
    mixAtt = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, "mix", mixSlider);

    // ---- Input Gain ----
    inputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (inputGainSlider);
    inputGainAtt = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, "inputGain", inputGainSlider);

    // ---- Output Gain ----
    outputGainSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (outputGainSlider);
    outputGainAtt = std::make_unique<SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputGainSlider);

    // ---- Rectifier ----
    rectifierBox.addItemList ({ "Germanium 1mF", "Germanium 2mF", "LED 3.3mF",
                                "Silicon 330nF", "Germanium 220nF",
                                "Silicon/Germanium 100nF" }, 1);
    addAndMakeVisible (rectifierBox);
    rectifierAtt = std::make_unique<ComboBoxAttachment> (
        audioProcessor.apvts, "rectifier", rectifierBox);

    // ---- Attack ----
    attackBox.addItemList ({ "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" }, 1);
    addAndMakeVisible (attackBox);
    attackAtt = std::make_unique<ComboBoxAttachment> (
        audioProcessor.apvts, "attack", attackBox);

    // ---- Release ----
    releaseBox.addItemList ({ "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" }, 1);
    addAndMakeVisible (releaseBox);
    releaseAtt = std::make_unique<ComboBoxAttachment> (
        audioProcessor.apvts, "release", releaseBox);

    // ---- Sidechain HPF ----
    addAndMakeVisible (sidechainHPFToggle);
    sidechainHPFAtt = std::make_unique<ButtonAttachment> (
        audioProcessor.apvts, "sidechainHPF", sidechainHPFToggle);

    setSize (700, 420);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() = default;

//==============================================================================
void IronCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e1e));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawFittedText ("SPL IRON – Parallel Tube Compressor Model",
                      getLocalBounds().removeFromTop (30), juce::Justification::centred, 1);
}

void IronCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (35); // title

    const int knobSize = 80;
    const int knobSpacing = 10;

    // Row 1: Threshold, Ratio, Bias, Mix
    auto row1 = area.removeFromTop (knobSize + 20);
    thresholdSlider.setBounds (row1.removeFromLeft (knobSize));
    row1.removeFromLeft (knobSpacing);
    ratioSlider.setBounds (row1.removeFromLeft (knobSize));
    row1.removeFromLeft (knobSpacing);
    biasSlider.setBounds (row1.removeFromLeft (knobSize));
    row1.removeFromLeft (knobSpacing);
    mixSlider.setBounds (row1.removeFromLeft (knobSize));

    area.removeFromTop (10);

    // Row 2: Input Gain, Output Gain, Sidechain HPF
    auto row2 = area.removeFromTop (knobSize + 20);
    inputGainSlider.setBounds (row2.removeFromLeft (knobSize));
    row2.removeFromLeft (knobSpacing);
    outputGainSlider.setBounds (row2.removeFromLeft (knobSize));
    row2.removeFromLeft (knobSpacing * 2);
    sidechainHPFToggle.setBounds (row2.removeFromLeft (180).withHeight (24));

    area.removeFromTop (10);

    // Row 3: Rectifier, Attack, Release
    auto row3 = area.removeFromTop (24);
    rectifierBox.setBounds (row3.removeFromLeft (160));
    row3.removeFromLeft (knobSpacing);
    attackBox.setBounds (row3.removeFromLeft (100));
    row3.removeFromLeft (knobSpacing);
    releaseBox.setBounds (row3.removeFromLeft (100));
}