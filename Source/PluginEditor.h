#pragma once
#include <juceHeader.h>
#include "PluginProcessor.h"

// === 1. LookAndFeel для ручек с картинкой и цифрами ===
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
            int numFrames = 101; // 101 кадр
            int frameHeight = filmstrip.getHeight() / numFrames;
            int frameWidth = filmstrip.getWidth();
            int frameIndex = (int)std::round(sliderPos * (numFrames - 1));

            bool hasLabels = slider.getProperties().contains("labels");
            
            int drawX = x, drawY = y, drawW = width, drawH = height;
            float textRadius = 0.0f;
            
            if (hasLabels)
            {
                int margin = 10;
                drawW = juce::jmin(width, height) - margin * 2;
                drawH = drawW;
                drawX = x + (width - drawW) / 2;
                drawY = y + (height - drawH) / 2;
                textRadius = drawW / 2.0f + 8.0f;
            }

            g.drawImage(filmstrip, 
                        drawX, drawY, drawW, drawH,               
                        0, frameIndex * frameHeight,       
                        frameWidth, frameHeight);          
                        
            if (hasLabels)
            {
                juce::String labelsStr = slider.getProperties()["labels"].toString();
                juce::StringArray labels;
                labels.addTokens(labelsStr, ",", "");
                
                int numLabels = labels.size();
                if (numLabels > 1)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.8f));
                    g.setFont(10.0f);
                    
                    float centerX = drawX + drawW / 2.0f;
                    float centerY = drawY + drawH / 2.0f;
                    
                    for (int i = 0; i < numLabels; ++i)
                    {
                        float angle = rotaryStartAngle + (float)i / (numLabels - 1) * (rotaryEndAngle - rotaryStartAngle);
                        float textX = centerX + std::cos(angle) * textRadius;
                        float textY = centerY + std::sin(angle) * textRadius;
                        
                        g.drawText(labels[i], textX - 10.0f, textY - 10.0f, 20.0f, 20.0f, juce::Justification::centred);
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

// === 2. LookAndFeel для тумблера Tube Bias (LOW/MID/HIGH) ===
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
        g.fillRoundedRectangle(centerX - 5.0f, bounds.getY() + 10.0f, 10.0f, bounds.getHeight() - 20.0f, 5.0f);
        
        float knobY = sliderPos; 
        juce::Rectangle<float> knobRect(centerX - 14.0f, knobY - 12.0f, 28.0f, 24.0f);
        
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(knobRect, 4.0f);
        
        g.setColour(juce::Colours::black);
        g.drawLine(centerX, knobY, centerX, knobY + 6.0f, 2.0f);
        
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText("HIGH", bounds.removeFromTop(15), juce::Justification::centred);
        g.drawText("MID", bounds.removeFromTop(bounds.getHeight() / 2.0f), juce::Justification::centred);
        g.drawText("LOW", bounds.removeFromBottom(15), juce::Justification::centred);
    }
};

// === 3. LookAndFeel для 3-позиционного тумблера (Air/Tape) ===
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
        g.fillRoundedRectangle(centerX - 5.0f, bounds.getY() + 10.0f, 10.0f, bounds.getHeight() - 20.0f, 5.0f);
        
        float knobY = sliderPos; 
        juce::Rectangle<float> knobRect(centerX - 14.0f, knobY - 12.0f, 28.0f, 24.0f);
        
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(knobRect, 4.0f);
        
        g.setColour(juce::Colours::black);
        g.drawLine(centerX, knobY, centerX, knobY + 6.0f, 2.0f);
        
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText("TAPE", bounds.removeFromTop(15), juce::Justification::centred);
        g.drawText("BYPASS", bounds.removeFromTop(bounds.getHeight() / 2.0f), juce::Justification::centred);
        g.drawText("AIR", bounds.removeFromBottom(15), juce::Justification::centred);
    }
};

// === 4. LookAndFeel для кнопок с картинкой ===
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

// === 5. Улучшенный Аналоговый VU-метр (Векторный) ===
class AnalogVUMeter : public juce::Component
{
public:
    AnalogVUMeter() {}
    void setLevel(float newLevel) { targetLevel = juce::jlimit(-20.0f, 10.0f, newLevel); }
    void setMode(int newMode) { currentMode = newMode; repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Желтый градиентный фон
        juce::ColourGradient grad(juce::Colour(0xffffe066), bounds.getCentreX(), bounds.getY(),
                                  juce::Colour(0xffd1a300), bounds.getCentreX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, 10.0f);
        
        // Внутренняя тень
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.drawRoundedRectangle(bounds.reduced(2.0f), 8.0f, 2.0f);

        float centerX = getWidth() / 2.0f;
        float centerY = getHeight() - 10.0f;
        float radius = getHeight() - 30.0f;
        float minAngle = juce::MathConstants<float>::pi * 1.2f;
        float maxAngle = juce::MathConstants<float>::pi * 1.8f;

        // Красная зона
        if (currentMode > 0) {
            juce::Path redZone;
            float startAngle = minAngle + juce::jmap(0.0f, -20.0f, (currentMode == 1 ? 3.0f : 10.0f), 0.0f, 1.0f) * (maxAngle - minAngle);
            redZone.addPieSegment(centerX - radius, centerY - radius, radius * 2, radius * 2, startAngle, maxAngle, 0.3f);
            g.setColour(juce::Colour(0xffcc0000));
            g.fillPath(redZone);
        }

        // Риски и цифры
        g.setColour(juce::Colours::black);
        g.setFont(11.0f);
        
        int numTicks = 20;
        for (int i = 0; i <= numTicks; ++i) {
            float angle = minAngle + (float)i / numTicks * (maxAngle - minAngle);
            float tickRadius = radius * (i % 4 == 0 ? 0.85f : 0.9f);
            
            float tickX1 = centerX + std::cos(angle) * radius;
            float tickY1 = centerY + std::sin(angle) * radius;
            float tickX2 = centerX + std::cos(angle) * tickRadius;
            float tickY2 = centerY + std::sin(angle) * tickRadius;
            
            g.drawLine(tickX1, tickY1, tickX2, tickY2, (i % 4 == 0) ? 2.0f : 1.0f);
            
            if (i % 4 == 0) {
                float textRadius = radius - 18.0f;
                float textX = centerX + std::cos(angle) * textRadius;
                float textY = centerY + std::sin(angle) * textRadius;
                
                juce::String text;
                if (currentMode == 0) text = juce::String(-20 + (i / 4) * 4);
                else if (currentMode == 1) text = juce::String(-20 + (i / 4) * 5);
                else text = juce::String(-20 + (i / 4) * 6);
                
                g.drawText(text, textX - 12.0f, textY - 10.0f, 24.0f, 20.0f, juce::Justification::centred);
            }
        }

        // Логотип VU
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("VU", centerX - 20.0f, centerY - 35.0f, 40.0f, 20.0f, juce::Justification::centred);
        
        // Стрелка
        juce::Path needle;
        float normalized = 0.0f;
        if (currentMode == 0) {
            normalized = 1.0f - juce::jmap(juce::jlimit(-20.0f, 0.0f, currentLevel), -20.0f, 0.0f, 0.0f, 1.0f);
        } else if (currentMode == 1) {
            normalized = juce::jmap(juce::jlimit(-20.0f, 3.0f, currentLevel), -20.0f, 3.0f, 0.0f, 1.0f);
        } else {
            normalized = juce::jmap(juce::jlimit(-20.0f, 10.0f, currentLevel), -20.0f, 10.0f, 0.0f, 1.0f);
        }

        float angle = minAngle + normalized * (maxAngle - minAngle);
        
        // Тень стрелки
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawLine(centerX + 2.0f, centerY + 2.0f, 
                   centerX + std::cos(angle) * radius + 2.0f, 
                   centerY + std::sin(angle) * radius + 2.0f, 3.0f);
                   
        // Стрелка
        g.setColour(juce::Colour(0xff222222));
        g.drawLine(centerX, centerY, 
                   centerX + std::cos(angle) * radius, 
                   centerY + std::sin(angle) * radius, 2.5f);
        
        // Крепление
        g.setColour(juce::Colours::black);
        g.fillEllipse(centerX - 8.0f, centerY - 8.0f, 16.0f, 16.0f);
        g.setColour(juce::Colour(0xff555555));
        g.fillEllipse(centerX - 4.0f, centerY - 4.0f, 8.0f, 8.0f);
    }
    void resized() override {}
    void tick() { currentLevel += (targetLevel - currentLevel) * 0.2f; repaint(); }
private:
    float currentLevel = 0.0f;
    float targetLevel = 0.0f;
    int currentMode = 0;
};

// === 6. Главный редактор ===
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
    BiasSwitchLookAndFeel biasLnf;
    ThreeWaySwitchLookAndFeel switchLnf;
    CustomImageToggleLookAndFeel imageToggleLnf;

    AnalogVUMeter vuMeter_L;
    AnalogVUMeter vuMeter_R;
    juce::ComboBox vuSwitch_L, vuSwitch_R;
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
