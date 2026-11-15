#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AnalogVUMeter : public juce::Component
{
public:
    AnalogVUMeter(const juce::String& title) : meterTitle(title) {}

    void setLevel(float newLevel) { targetLevel = juce::jlimit(-20.0f, 0.0f, newLevel); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colour(0xffffcc00));
        g.fillRoundedRectangle(bounds, 10.0f);
        g.setColour(juce::Colours::black);
        g.setFont(12.0f);
        g.drawText(meterTitle, bounds.removeFromTop(20.0f), juce::Justification::centred);
        
        float centerX = getWidth() / 2.0f;
        float centerY = getHeight() - 10.0f;
        float radius = getHeight() - 30.0f;

        float minAngle = juce::MathConstants<float>::pi * 1.2f;
        float maxAngle = juce::MathConstants<float>::pi * 1.8f;

        g.setFont(10.0f);
        for (int db = 0; db >= -20; db -= 5) {
            float normalized = juce::jmap((float)db, -20.0f, 0.0f, 0.0f, 1.0f);
            float angle = minAngle + normalized * (maxAngle - minAngle);
            
            float tickX = centerX + std::cos(angle) * (radius + 5.0f);
            float tickY = centerY + std::sin(angle) * (radius + 5.0f);
            g.fillEllipse(tickX - 2.0f, tickY - 2.0f, 4.0f, 4.0f);
            
            float textX = centerX + std::cos(angle) * (radius - 15.0f);
            float textY = centerY + std::sin(angle) * (radius - 15.0f);
            g.drawText(juce::String(db), textX - 10.0f, textY - 10.0f, 20.0f, 20.0f, juce::Justification::centred);
        }
        
        juce::Path needle;
        float normalized = juce::jmap(currentLevel, -20.0f, 0.0f, 0.0f, 1.0f);
        float angle = minAngle + normalized * (maxAngle - minAngle);
        needle.startNewSubPath(centerX, centerY);
        needle.lineTo(centerX + std::cos(angle) * radius, centerY + std::sin(angle) * radius);
        
        g.setColour(juce::Colour(0xffcc0000));
        g.strokePath(needle, juce::PathStrokeType(3.0f));
        g.fillEllipse(centerX - 6.0f, centerY - 6.0f, 12.0f, 12.0f);
    }

    void resized() override {}
    void tick() { currentLevel += (targetLevel - currentLevel) * 0.2f; repaint(); }

private:
    juce::String meterTitle;
    float currentLevel = 0.0f;
    float targetLevel = 0.0f;
};

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
        setColour (juce::ToggleButton::textColourId, juce::Colours::white);
        setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffe65100));
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

    AnalogVUMeter vuMeter_L {"GR L"};
    AnalogVUMeter vuMeter_R {"GR R"};

    // --- TMT UI Элементы ---
    juce::ToggleButton tmtEnableToggle { "ON" }; // НОВАЯ КНОПКА
    juce::TextButton tmtModeButton{"A"};
    juce::TextButton tmtLeftBtn{"<"};
    juce::TextButton tmtRightBtn{">"};
    juce::TextButton tmtRandomBtn{"RND"};
    juce::Label tmtDisplay;
    juce::Label tmtTitleLabel;

    // --- Слайдеры и комбо ---
    juce::Slider thresholdSlider_L, inputGainSlider_L;
    juce::ComboBox biasBox_L, rectifierBox_L, attackBox_L, releaseBox_L;
    juce::Label labelThreshold_L, labelInput_L, labelBias_L, labelRectifier_L, labelAttack_L, labelRelease_L;

    juce::Slider thresholdSlider_R, inputGainSlider_R;
    juce::ComboBox biasBox_R, rectifierBox_R, attackBox_R, releaseBox_R;
    juce::Label labelThreshold_R, labelInput_R, labelBias_R, labelRectifier_R, labelAttack_R, labelRelease_R;

    juce::Slider outputGainSlider, mixSlider, widthSlider, monoSlider, headroomSlider;
    juce::Label labelOutput, labelMix, labelWidth, labelMono, labelHeadroom;
    juce::ComboBox sidechainBox, airBassBox;
    juce::Label labelSidechain, labelAirBass;

    juce::ToggleButton linkButton { "Link L/R" };
    juce::ToggleButton midSideButton { "M/S Mode" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> attThreshold_L, attInput_L, attThreshold_R, attInput_R;
    std::unique_ptr<SliderAttachment> attOutput, attMix, attWidth, attMono, attHeadroom;
    std::unique_ptr<ComboBoxAttachment> attBias_L, attRectifier_L, attAttack_L, attRelease_L;
    std::unique_ptr<ComboBoxAttachment> attBias_R, attRectifier_R, attAttack_R, attRelease_R;
    std::unique_ptr<ComboBoxAttachment> attSidechain, attAirBass;
    std::unique_ptr<ButtonAttachment> attLink, attMidSide, attTMTMode, attTMTEnable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};
