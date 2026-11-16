#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// === LookAndFeel для текста с тенью ===
class ShadowLabelLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll(label.findColour(juce::Label::backgroundColourId));
        if (!label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const juce::Font font(label.getFont());
            auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
            
            g.setColour(juce::Colours::black.withAlpha(0.9f));
            g.setFont(font);
            g.drawFittedText(label.getText(), textArea.translated(1, 1), label.getJustificationType(), 1, 0.5f);
            
            g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
            g.setFont(font);
            g.drawFittedText(label.getText(), textArea, label.getJustificationType(), 1, 0.5f);
        }
    }
};

// === LookAndFeel для ручек ===
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
            int numFrames = 101; 
            int frameHeight = filmstrip.getHeight() / numFrames;
            int frameWidth = filmstrip.getWidth();
            int frameIndex = (int)std::round(sliderPos * (numFrames - 1));

            bool hasLabels = slider.getProperties().contains("labels");
            
            int size = juce::jmin(width, height);
            int drawW = hasLabels ? size - 24 : size;
            int drawH = hasLabels ? size - 24 : size;
            int drawX = x + (width - drawW) / 2;
            int drawY = y + (height - drawH) / 2;

            float textRadius = drawW / 2.0f + 14.0f;

            g.drawImage(filmstrip, drawX, drawY, drawW, drawH, 0, frameIndex * frameHeight, frameWidth, frameHeight);          
                        
            if (hasLabels)
            {
                juce::String labelsStr = slider.getProperties()["labels"].toString();
                juce::StringArray labels;
                labels.addTokens(labelsStr, ",", "");
                int numLabels = labels.size();
                if (numLabels > 1)
                {
                    g.setFont(10.0f);
                    float centerX = drawX + drawW / 2.0f;
                    float centerY = drawY + drawH / 2.0f;
                    
                    for (int i = 0; i < numLabels; ++i)
                    {
                        float angle = rotaryStartAngle + (float)i / (numLabels - 1) * (rotaryEndAngle - rotaryStartAngle);
                        float textX = centerX + std::cos(angle) * textRadius;
                        float textY = centerY + std::sin(angle) * textRadius;
                        
                        g.setColour(juce::Colours::black);
                        g.drawText(labels[i], textX - 20.0f + 1, textY - 10.0f + 1, 40.0f, 20.0f, juce::Justification::centred);
                        g.setColour(juce::Colours::white);
                        g.drawText(labels[i], textX - 20.0f, textY - 10.0f, 40.0f, 20.0f, juce::Justification::centred);
                    }
                }
            }
        }
        else
        {
            g.setColour(juce::Colour(0xff1e1e1e));
            g.fillEllipse(x, y, width, height);
        }
    }
};

// === LookAndFeel для тумблера Bias (HIGH/LOW) ===
class BiasSwitchLookAndFeel : public juce::LookAndFeel_V4
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
        g.fillRoundedRectangle(centerX - 5.0f, bounds.getY() + 25.0f, 10.0f, bounds.getHeight() - 50.0f, 5.0f);
        
        float knobY = sliderPos; 
        juce::Rectangle<float> knobRect(centerX - 14.0f, knobY - 12.0f, 28.0f, 24.0f);
        
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(knobRect, 4.0f);
        g.setColour(juce::Colours::black);
        g.drawLine(centerX, knobY, centerX, knobY + 6.0f, 2.0f);
        
        g.setFont(11.0f);
        g.setColour(juce::Colours::white);
        g.drawText("HIGH", bounds.getX(), bounds.getY(), bounds.getWidth(), 20, juce::Justification::centred);
        g.drawText("LOW", bounds.getX(), bounds.getBottom() - 20, bounds.getWidth(), 20, juce::Justification::centred);
    }
};

// === LookAndFeel для тумблера Air/Tape ===
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
        g.fillRoundedRectangle(centerX - 5.0f, bounds.getY() + 25.0f, 10.0f, bounds.getHeight() - 50.0f, 5.0f);
        
        float knobY = sliderPos; 
        juce::Rectangle<float> knobRect(centerX - 14.0f, knobY - 12.0f, 28.0f, 24.0f);
        
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(knobRect, 4.0f);
        g.setColour(juce::Colours::black);
        g.drawLine(centerX, knobY, centerX, knobY + 6.0f, 2.0f);
        
        g.setFont(10.0f);
        g.setColour(juce::Colours::white);
        g.drawText("TAPE", bounds.getX(), bounds.getY(), bounds.getWidth(), 20, juce::Justification::centred);
        g.drawText("BYPASS", bounds.getX(), bounds.getCentreY() - 10, bounds.getWidth(), 20, juce::Justification::centred);
        g.drawText("AIR", bounds.getX(), bounds.getBottom() - 20, bounds.getWidth(), 20, juce::Justification::centred);
    }
};

// === LookAndFeel для кнопок ===
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
            g.drawImage(switchImg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 0, frameY, switchImg.getWidth(), frameHeight);
        }
        else
        {
            g.setColour(juce::Colours::grey);
            g.fillRect(button.getLocalBounds());
        }
    }
};

// === VU-метр ===
class AnalogVUMeter : public juce::Component
{
public:
    AnalogVUMeter() {}
    void setLevel(float newLevel) { targetLevel = juce::jlimit(-20.0f, 0.0f, newLevel); }

    void paint(juce::Graphics& g) override
    {
        static juce::Image meterImg = juce::ImageFileFormat::loadFrom(
            BinaryData::gr_meter_filmstrip_png, 
            BinaryData::gr_meter_filmstrip_pngSize
        );

        if (meterImg.isValid()) 
        {
            int numFrames = 101; 
            int frameHeight = meterImg.getHeight() / numFrames;
            int frameWidth = meterImg.getWidth();
            
            float normalized = juce::jmap(juce::jlimit(-20.0f, 0.0f, currentLevel), -20.0f, 0.0f, 1.0f, 0.0f);
            int frameIndex = (int)std::round(normalized * (numFrames - 1));
            
            auto bounds = getLocalBounds().toFloat();
            
            float targetW = bounds.getWidth();
            float targetH = bounds.getHeight();
            float aspect = (float)frameWidth / (float)frameHeight;
            
            float drawW = targetW;
            float drawH = targetW / aspect;
            if (drawH > targetH) {
                drawH = targetH;
                drawW = targetH * aspect;
            }
            
            float drawX = bounds.getX() + (targetW - drawW) * 0.5f;
            float drawY = bounds.getY() + (targetH - drawH) * 0.5f;
            
            g.drawImage(meterImg, 
                        (int)drawX, (int)drawY, (int)drawW, (int)drawH,
                        0, frameIndex * frameHeight, frameWidth, frameHeight);
        }
        else
        {
            g.fillAll(juce::Colours::orange);
        }
    }
    void resized() override {}
    void tick() { currentLevel += (targetLevel - currentLevel) * 0.2f; repaint(); }
private:
    float currentLevel = 0.0f;
    float targetLevel = 0.0f;
};

// === Главный редактор ===
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
    
    ShadowLabelLookAndFeel shadowLnf;
    FilmstripLookAndFeel filmstripLnf;
    BiasSwitchLookAndFeel biasLnf;
    ThreeWaySwitchLookAndFeel switchLnf;
    CustomImageToggleLookAndFeel imageToggleLnf;

    AnalogVUMeter vuMeter_L;
    AnalogVUMeter vuMeter_R;
    
    juce::ComboBox presetBox;

    juce::ToggleButton tmtEnableToggle { "" }; 
    juce::Label labelTmtOn;
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
    juce::Label labelLink;
    juce::ToggleButton midSideButton { "" };
    juce::Label labelMidSide;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> attThreshold_L, attInput_L, attOutput_L, attAttack_L, attRelease_L, attBias_L, attRectifier_L, attSidechain_L, attAirBass_L;
    std::unique_ptr<SliderAttachment> attThreshold_R, attInput_R, attOutput_R, attAttack_R, attRelease_R, attBias_R, attRectifier_R, attSidechain_R, attAirBass_R;
    std::unique_ptr<SliderAttachment> attMix, attWidth, attMono, attHeadroom;
    std::unique_ptr<ButtonAttachment> attLink, attMidSide, attTMTMode, attTMTEnable;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessorEditor)
};
