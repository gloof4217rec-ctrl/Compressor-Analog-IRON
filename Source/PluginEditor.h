#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class IronLookAndFeel : public juce::LookAndFeel_V4 {
public:
    IronLookAndFeel() {
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe65100));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff333333));
        setColour (juce::Slider::thumbColourId, juce::Colours::white);
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff222222));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff555555));
        setColour (juce::Label::textColourId, juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (juce::Colour (0xff1e1e1e));
        g.fillEllipse (rx, ry, rw, rw);
        g.setColour (juce::Colour (0xff444444));
        g.drawEllipse (rx, ry, rw, rw, 2.0f);

        juce::Path p;
        auto pointerLength = radius * 0.6f;
        auto pointerThickness = 3.0f;
        p.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        
        g.setColour (juce::Colour (0xffe65100));
        g.fillPath (p);
    }
};

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

    juce::Slider inputGainSlider, outputGainSlider, thresholdSlider, mixSlider;
    juce::Slider widthSlider, monoSlider;
    
    juce::ComboBox biasLBox, biasRBox, rectifierBox, attackBox, releaseBox, sidechainBox, airBassBox;
    juce::Label inputLabel, outputLabel, thresholdLabel, mixLabel, widthLabel, monoLabel;
    juce::Label biasLLabel, biasRLabel, rectifierLabel, attackLabel, releaseLabel, sidechainLabel, airBassLabel;

    juce::Label grDisplay;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> inputAtt, outputAtt, thresholdAtt, mixAtt, widthAtt, monoAtt;
    std::unique_ptr<ComboBoxAttachment> biasLAtt, biasRAtt, rectifierAtt, attackAtt, releaseAtt, sidechainAtt, airBassAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};
