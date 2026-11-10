#pragma once

#include <JuceHeader.h>
#include <array>

class IronCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    IronCompressorAudioProcessor();
    ~IronCompressorAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double sr = 44100.0;
    std::array<float, 2> envelope { 0.0f, 0.0f };

    // Фильтры боковой цепи (Sidechain)
    juce::dsp::IIR::Filter<float> sidechainHPF[2];
    juce::dsp::IIR::Filter<float> sidechainPreset[2];

    juce::SmoothedValue<float> inputGainSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;
    
    // Для отслеживания изменений параметров
    int lastSidechainMode = -1;

    void updateTimingCoefficients();
    static float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessor)
};
