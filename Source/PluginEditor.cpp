#include "PluginEditor.h"

IronCompressorAudioProcessorEditor::IronCompressorAudioProcessorEditor (IronCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&ironLnf);
    startTimerHz(30);

    addAndMakeVisible(vuMeter_L);
    addAndMakeVisible(vuMeter_R);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    auto setupCombo = [this](juce::ComboBox& c, juce::Label& l, const juce::String& name, const juce::StringArray& items) {
        c.addItemList (items, 1);
        addAndMakeVisible (c);
        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setupSlider(thresholdSlider_L, labelThreshold_L, "THRESHOLD L");
    setupSlider(inputGainSlider_L, labelInput_L, "INPUT L");
    setupCombo(biasBox_L, labelBias_L, "BIAS L", { "Low", "Mid", "High" });
    setupCombo(rectifierBox_L, labelRectifier_L, "RECTIFIER L", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox_L, labelAttack_L, "ATTACK L", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox_L, labelRelease_L, "RELEASE L", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });

    setupSlider(thresholdSlider_R, labelThreshold_R, "THRESHOLD R");
    setupSlider(inputGainSlider_R, labelInput_R, "INPUT R");
    setupCombo(biasBox_R, labelBias_R, "BIAS R", { "Low", "Mid", "High" });
    setupCombo(rectifierBox_R, labelRectifier_R, "RECTIFIER R", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" });
    setupCombo(attackBox_R, labelAttack_R, "ATTACK R", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" });
    setupCombo(releaseBox_R, labelRelease_R, "RELEASE R", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" });

    setupSlider(outputGainSlider, labelOutput, "OUTPUT");
    setupSlider(mixSlider, labelMix, "MIX");
    setupSlider(widthSlider, labelWidth, "WIDTH");
    setupSlider(monoSlider, labelMono, "MONO");
    setupSlider(headroomSlider, labelHeadroom, "HEADROOM");

    setupCombo(sidechainBox, labelSidechain, "SC EQ", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" });
    setupCombo(airBassBox, labelAirBass, "AIR / TAPE", { "Bypass", "AirBass", "Tape Roll-Off" });

    addAndMakeVisible(linkButton);
    addAndMakeVisible(midSideButton);

    attThreshold_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdL", thresholdSlider_L);
    attInput_L = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainL", inputGainSlider_L);
    attBias_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "biasL", biasBox_L);
    attRectifier_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "rectifierL", rectifierBox_L);
    attAttack_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "attackL", attackBox_L);
    attRelease_L = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "releaseL", releaseBox_L);

    attThreshold_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "thresholdR", thresholdSlider_R);
    attInput_R = std::make_unique<SliderAttachment>(audioProcessor.apvts, "inputGainR", inputGainSlider_R);
    attBias_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "biasR", biasBox_R);
    attRectifier_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "rectifierR", rectifierBox_R);
    attAttack_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "attackR", attackBox_R);
    attRelease_R = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "releaseR", releaseBox_R);

    attOutput = std::make_unique<SliderAttachment>(audioProcessor.apvts, "outputGain", outputGainSlider);
    attMix = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    attWidth = std::make_unique<SliderAttachment>(audioProcessor.apvts, "stereoWidth", widthSlider);
    attMono = std::make_unique<SliderAttachment>(audioProcessor.apvts, "monoMaker", monoSlider);
    attHeadroom = std::make_unique<SliderAttachment>(audioProcessor.apvts, "headroom", headroomSlider);
    attSidechain = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "sidechainEq", sidechainBox);
    attAirBass = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "airBass", airBassBox);

    attLink = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "link", linkButton);
    attMidSide = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "midSide", midSideButton);

    setSize (1000, 600);
}

IronCompressorAudioProcessorEditor::~IronCompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void IronCompressorAudioProcessorEditor::timerCallback()
{
    vuMeter_L.setLevel(audioProcessor.currentGR_dB_L.load());
    vuMeter_R.setLevel(audioProcessor.currentGR_dB_R.load());
    
    vuMeter_L.tick();
    vuMeter_R.tick();
}

void IronCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff7a0000));
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRect (0, 0, getWidth(), 60);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("SPL IRON", 20, 10, 200, 40, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xffaaaaaa));
    g.setFont (14.0f);
    g.drawText ("Parallel Dual-Tube Compressor", 160, 15, 300, 30, juce::Justification::centredLeft);
}

void IronCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (60);

    const int knobSize = 70;
    const int comboHeight = 25;
    const int labelHeight = 18;

    auto leftSection = area.removeFromLeft (area.getWidth() / 2 - 10);
    leftSection.reduce (10, 0);

    vuMeter_L.setBounds (leftSection.removeFromTop(60).withSizeKeepingCentre(150, 60));
    leftSection.removeFromTop(10);

    auto leftRow1 = leftSection.removeFromTop (110);
    auto layoutLeftKnob = [&](juce::Slider& s, juce::Label& l, int width) {
        auto itemArea = leftRow1.removeFromLeft (width);
        l.setBounds (itemArea.removeFromTop (labelHeight));
        s.setBounds (itemArea.withSizeKeepingCentre (knobSize, knobSize + 20));
    };
    layoutLeftKnob(thresholdSlider_L, labelThreshold_L, leftSection.getWidth() / 2);
    layoutLeftKnob(inputGainSlider_L, labelInput_L, leftSection.getWidth() / 2);

    leftSection.removeFromTop (10);
    auto leftRow2 = leftSection.removeFromTop (80);
    int leftComboWidth = leftSection.getWidth() / 2;
    auto layoutLeftCombo = [&](juce::ComboBox& c, juce::Label& l) {
        auto itemArea = leftRow2.removeFromLeft (leftComboWidth);
        itemArea.reduce (5, 0);
        l.setBounds (itemArea.removeFromTop (labelHeight));
        c.setBounds (itemArea.removeFromTop (comboHeight));
    };
    layoutLeftCombo(biasBox_L, labelBias_L);
    layoutLeftCombo(rectifierBox_L, labelRectifier_L);

    leftSection.removeFromTop (10);
    auto leftRow3 = leftSection.removeFromTop (80);
    layoutLeftCombo(attackBox_L, labelAttack_L);
    layoutLeftCombo(releaseBox_L, labelRelease_L);

    auto rightSection = area;
    rightSection.reduce (10, 0);

    vuMeter_R.setBounds (rightSection.removeFromTop(60).withSizeKeepingCentre(150, 60));
    rightSection.removeFromTop(10);

    auto rightRow1 = rightSection.removeFromTop (110);
    auto layoutRightKnob = [&](juce::Slider& s, juce::Label& l, int width) {
        auto itemArea = rightRow1.removeFromLeft (width);
        l.setBounds (itemArea.removeFromTop (labelHeight));
        s.setBounds (itemArea.withSizeKeepingCentre (knobSize, knobSize + 20));
    };
    layoutRightKnob(thresholdSlider_R, labelThreshold_R, rightSection.getWidth() / 2);
    layoutRightKnob(inputGainSlider_R, labelInput_R, rightSection.getWidth() / 2);

    rightSection.removeFromTop (10);
    auto rightRow2 = rightSection.removeFromTop (80);
    int rightComboWidth = rightSection.getWidth() / 2;
    auto layoutRightCombo = [&](juce::ComboBox& c, juce::Label& l) {
        auto itemArea = rightRow2.removeFromLeft (rightComboWidth);
        itemArea.reduce (5, 0);
        l.setBounds (itemArea.removeFromTop (labelHeight));
        c.setBounds (itemArea.removeFromTop (comboHeight));
    };
    layoutRightCombo(biasBox_R, labelBias_R);
    layoutRightCombo(rectifierBox_R, labelRectifier_R);

    rightSection.removeFromTop (10);
    auto rightRow3 = rightSection.removeFromTop (80);
    layoutRightCombo(attackBox_R, labelAttack_R);
    layoutRightCombo(releaseBox_R, labelRelease_R);

    auto bottomSection = getLocalBounds().removeFromBottom (180).reduced (20);
    auto bottomRow1 = bottomSection.removeFromTop (100);

    int bottomKnobWidth = bottomSection.getWidth() / 5;
    auto layoutBottomKnob = [&](juce::Slider& s, juce::Label& l) {
        auto itemArea = bottomRow1.removeFromLeft (bottomKnobWidth);
        l.setBounds (itemArea.removeFromTop (labelHeight));
        s.setBounds (itemArea.withSizeKeepingCentre (knobSize, knobSize + 20));
    };
    layoutBottomKnob(outputGainSlider, labelOutput);
    layoutBottomKnob(mixSlider, labelMix);
    layoutBottomKnob(widthSlider, labelWidth);
    layoutBottomKnob(monoSlider, labelMono);
    layoutBottomKnob(headroomSlider, labelHeadroom);

    auto switchArea = bottomSection.removeFromTop (40);
    switchArea = switchArea.withSizeKeepingCentre (400, 40);
    linkButton.setBounds(switchArea.removeFromLeft(200).reduced(10, 0));
    midSideButton.setBounds(switchArea.removeFromLeft(200).reduced(10, 0));

    auto bottomRow2 = bottomSection.removeFromTop (80);
    int bottomComboWidth = bottomSection.getWidth() / 2;
    auto layoutBottomCombo = [&](juce::ComboBox& c, juce::Label& l) {
        auto itemArea = bottomRow2.removeFromLeft (bottomComboWidth);
        itemArea.reduce (10, 0);
        l.setBounds (itemArea.removeFromTop (labelHeight));
        c.setBounds (itemArea.removeFromTop (comboHeight));
    };
    layoutBottomCombo(sidechainBox, labelSidechain);
    layoutBottomCombo(airBassBox, labelAirBass);
}
