#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// === LookAndFeel для ручек с картинкой (Фильмстрип) ===
class FilmstripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        static juce::Image filmstrip = juce::ImageFileFormat::loadFrom(
            BinaryData::knob_filmstrip_png, 
            BinaryData::knob_filmstrip_pngSize
        );

        if (filmstrip.isValid())
        {
            int numFrames = 64; 
            int frameHeight = filmstrip.getHeight() / numFrames;
            int frameWidth = filmstrip.getWidth();
            
            int frameIndex = (int)std::round(sliderPos * (numFrames - 1));
            
            g.drawImage(filmstrip, 
                        x, y, width, height,               
                        0, frameIndex * frameHeight,       
                        frameWidth, frameHeight);          
        }
        else
        {
            g.setColour(juce::Colour(0xff1e1e1e));
            g.fillEllipse(x, y, width, height);
        }
    }
};

// === LookAndFeel для 3-позиционного тумблера (Air/Tape) ===
class ThreeWaySwitchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearVertical) return;

        auto bounds = slider.getLocalBounds().toFloat();
        auto centerX = bounds.getCentreX();
        
        g.setColour(juce::Colour(0xff111111));
        g.fillRoundedRectangle(centerX - 4.0f, bounds.getY() + 10.0f, 8.0f, bounds.getHeight() - 20.0f, 4.0f);
        
        float knobY = sliderPos; 
        juce::Rectangle<float> knobRect(centerX - 12.0f, knobY - 10.0f, 24.0f, 20.0f);
        
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(knobRect, 3.0f);
        
        g.setColour(juce::Colours::black);
        g.drawLine(centerX, knobY, centerX, knobY + 6.0f, 2.0f);
        
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText("TAPE", bounds.removeFromTop(15), juce::Justification::centred);
        g.drawText("BYPASS", bounds.removeFromTop(15), juce::Justification::centred);
        g.drawText("AIR", bounds.removeFromBottom(15), juce::Justification::centred);
    }
};

// === LookAndFeel для кнопок с картинкой ===
class CustomImageToggleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        static juce::Image switchImg = juce::ImageFileFormat::loadFrom(
            BinaryData::toggle_switch_png, 
            BinaryData::toggle_switch_pngSize
        );

        if (switchImg.isValid())
        {
            int frameHeight = switchImg.getHeight() / 2;
            int frameY = button.getToggleState() ? 0 : frameHeight;
            
            auto bounds = button.getLocalBounds();
            g.drawImage(switchImg, 
                        bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                        0, frameY, switchImg.getWidth(), frameHeight);
        }
        else
        {
            g.setColour(juce::Colours::grey);
            g.fillRect(button.getLocalBounds());
        }
    }
};

// === Аналоговый VU-метр ===
class AnalogVUMeter : public juce::Component
{
public:
    AnalogVUMeter() {}
    void setLevel(float newLevel) { targetLevel = juce::jlimit(-20.0f, 10.0f, newLevel); }
    void setMode(int newMode) { currentMode = newMode; repaint(); }
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xffffcc00));
        g.fillRoundedRectangle(bounds, 10.0f);
        float centerX = getWidth() / 2.0f;
        float centerY = getHeight() - 10.0f;
        float radius = getHeight() - 30.0f;
        float minAngle = juce::MathConstants<float>::pi * 1.2f;
        float maxAngle = juce::MathConstants<float>::pi * 1.8f;
        
        g.setColour(juce::Colours::black);
        g.setFont(10.0f);
        for (int val = 0; val >= -20; val -= 5) {
            float normalized = juce::jmap((float)val, -20.0f, 0.0f, 0.0f, 1.0f);
            float angle = minAngle + normalized * (maxAngle - minAngle);
            float tickX = centerX + std::cos(angle) * (radius + 5.0f);
            float tickY = centerY + std::sin(angle) * (radius + 5.0f);
            g.fillEllipse(tickX - 2.0f, tickY - 2.0f, 4.0f, 4.0f);
            float textX = centerX + std::cos(angle) * (radius - 15.0f);
            float textY = centerY + std::sin(angle) * (radius - 15.0f);
            g.drawText(juce::String(val), textX - 10.0f, textY - 10.0f, 20.0f, 20.0f, juce::Justification::centred);
        }
        
        juce::Path needle;
        float currentVal = currentLevel;
        if (currentMode == 2) currentVal = juce::jlimit(-10.0f, 10.0f, currentVal);
        else currentVal = juce::jlimit(-20.0f, 0.0f, currentVal);
        float normalized = juce::jmap(currentVal, -20.0f, 0.0f, 0.0f, 1.0f);
        if (currentMode == 2) normalized = juce::jmap(currentVal, -10.0f, 10.0f, 0.0f, 1.0f);
        
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
    float currentLevel = 0.0f;
    float targetLevel = 0.0f;
    int currentMode = 0;
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
    
    FilmstripLookAndFeel filmstripLnf;
    ThreeWaySwitchLookAndFeel switchLnf;
    CustomImageToggleLookAndFeel imageToggleLnf;

    AnalogVUMeter vuMeter_L;
    AnalogVUMeter vuMeter_R;
    juce::ComboBox vuSwitch_L, vuSwitch_R;
    juce::ComboBox presetBox;

    juce::ToggleButton tmtEnableToggle { "" }; 
    juce::TextButton tmtModeButton{"A"};
    juce::TextButton tmtLeftBtn{"<"};
    juce::TextButton tmtRightBtn{">"};
    juce::TextButton tmtRandomBtn{"RND"};
    juce::Label tmtDisplay;

    juce::Slider thresholdSlider_L, inputGainSlider_L, outputGainSlider_L;
    juce::Slider attackSlider_L, releaseSlider_L, biasSlider_L, rectifierSlider_L, sidechainSlider_L, airBassSlider_L;
    juce::Label labelThreshold_L, labelInput_L, labelOutput_L, labelAttack_L, labelRelease_L, labelBias_L, labelRectifier_L, labelSidechain_L, labelAirBass_L;

    juce::Slider thresholdSlider_R, inputGainSlider_R, outputGainSlider_R;
    juce::Slider attackSlider_R, releaseSlider_R, biasSlider_R, rectifierSlider_R, sidechainSlider_R, airBassSlider_R;
    juce::Label labelThreshold_R, labelInput_R, labelOutput_R, labelAttack_R, labelRelease_R, labelBias_R, labelRectifier_R, labelSidechain_R, labelAirBass_R;

    juce::Slider mixSlider, widthSlider, monoSlider, headroomSlider;
    juce::Label labelMix, labelWidth, labelMono, labelHeadroom;

    juce::ToggleButton linkButton { "" };
    juce::ToggleButton midSideButton { "" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> attThreshold_L, attInput_L, attOutput_L, attAttack_L, attRelease_L, attBias_L, attRectifier_L, attSidechain_L, attAirBass_L;
    std::unique_ptr<SliderAttachment> attThreshold_R, attInput_R, attOutput_R, attAttack_R, attRelease_R, attBias_R, attRectifier_R, attSidechain_R, attAirBass_R;
    std::unique_ptr<SliderAttachment> attMix, attWidth, attMono, attHeadroom;
    std::unique_ptr<ButtonAttachment> attLink, attMidSide, attTMTMode, attTMTEnable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};
