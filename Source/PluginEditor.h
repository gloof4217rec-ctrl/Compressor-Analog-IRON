#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

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
    
    // Переменные для UI (Gain Reduction)
    std::atomic<float> currentGRL { 0.0f };
    std::atomic<float> currentGRR { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    double sr = 44100.0;
    std::array<float, 2> envelope { 0.0f, 0.0f };

    // Фильтры для Sidechain EQ, Mono Maker и AirBass
    juce::dsp::IIR::Filter<float> sidechainFilter[2];
    juce::dsp::IIR::Filter<float> monoMakerFilter[2];
    juce::dsp::IIR::Filter<float> airBassFilter[2];
    
    juce::SmoothedValue<float> inputGainSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;
    
    int lastSidechainMode = -1;
    int lastAirBassMode = -1;

    void updateTimingCoefficients();
    void updateFilters(int scMode, int airBassMode);
    
    static float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessor)
};
