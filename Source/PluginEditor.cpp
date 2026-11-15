#include "PluginEditor.h"

// ... (функция createTintedImage остается без изменений) ...

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Устанавливаем наши кастомные LookAndFeel для соответствующих элементов
    // (В реальном коде их нужно присваивать каждому слайдеру/кнопке отдельно)

    startTimerHz(30);

    addAndMakeVisible(vuMeter_L);
    addAndMakeVisible(vuMeter_R);

    // ... (настройка vuSwitch_L, vuSwitch_R, presetBox, TMT без изменений) ...

    // === Настройка ступенчатых слайдеров ===
    auto setupSteppedSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name, int numSteps) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(0.0, (double)(numSteps - 1), 1.0); // Дискретные шаги
        s.setLookAndFeel(&steppedLnf); // Применяем кастомный LookAndFeel
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    // Левый канал
    setupSteppedSlider(attackSlider_L, labelAttack_L, "ATTACK L", 6);
    setupSteppedSlider(releaseSlider_L, labelRelease_L, "RELEASE L", 6);
    setupSteppedSlider(biasSlider_L, labelBias_L, "BIAS L", 3);
    setupSteppedSlider(rectifierSlider_L, labelRectifier_L, "RECTIFIER L", 6);
    setupSteppedSlider(airBassSlider_L, labelAirBass_L, "AIR/TAPE L", 3);
    setupSteppedSlider(sidechainSlider_L, labelSidechain_L, "SC EQ L", 5);

    // Правый канал
    setupSteppedSlider(attackSlider_R, labelAttack_R, "ATTACK R", 6);
    setupSteppedSlider(releaseSlider_R, labelRelease_R, "RELEASE R", 6);
    setupSteppedSlider(biasSlider_R, labelBias_R, "BIAS R", 3);
    setupSteppedSlider(rectifierSlider_R, labelRectifier_R, "RECTIFIER R", 6);
    setupSteppedSlider(airBassSlider_R, labelAirBass_R, "AIR/TAPE R", 3);
    setupSteppedSlider(sidechainSlider_R, labelSidechain_R, "SC EQ R", 5);

    // ... (привязка attachments для новых слайдеров) ...
    attAttack_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attackL", attackSlider_L);
    attRelease_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "releaseL", releaseSlider_L);
    attBias_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "biasL", biasSlider_L);
    attRectifier_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "rectifierL", rectifierSlider_L);
    attAirBass_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "airBassL", airBassSlider_L);
    attSidechain_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "sidechainEqL", sidechainSlider_L);

    attAttack_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "attackR", attackSlider_R);
    attRelease_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "releaseR", releaseSlider_R);
    attBias_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "biasR", biasSlider_R);
    attRectifier_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "rectifierR", rectifierSlider_R);
    attAirBass_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "airBassR", airBassSlider_R);
    attSidechain_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "sidechainEqR", sidechainSlider_R);

    // Применяем LookAndFeel для тумблеров
    linkButton.setLookAndFeel(&toggleLnf);
    midSideButton.setLookAndFeel(&toggleLnf);
    tmtEnableToggle.setLookAndFeel(&toggleLnf);
    // ... (для vuSwitch_L/R тоже нужно будет применить toggleLnf, если они станут тумблерами) ...

    // ... (остальная настройка и setSize) ...
}

// ... (деструктор, timerCallback, paint, resized) ...

// ВАЖНО: В resized() нужно будет обновить координаты для новых слайдеров вместо ComboBox
void IronCompressorAudioProcessorEditor::resized()
{
    // ... (здесь вы располагаете новые слайдеры Attack/Release/Bias и т.д. по своим местам, как в предыдущей версии, но с учетом того, что это теперь квадратные ручки, а не вытянутые ComboBox) ...
}
