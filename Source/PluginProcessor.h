#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

struct TMTParams {
    float threshOffset = 0.0f;
    float timeOffset = 1.0f;   
    float biasOffset = 1.0f;   
};

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

    std::atomic<float> currentGR_dB_L { 0.0f };
    std::atomic<float> currentGR_dB_R { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateFilters();
    void updateTimingCoefficients(int attL, int relL, int recL, int attR, int relR, int recR);

    double sr = 44100.0;
    std::array<float, 2> envelope { 0.0f, 0.0f };

    juce::dsp::IIR::Filter<float> sidechainFilter[2];
    juce::dsp::IIR::Filter<float> monoMakerFilter[2];
    juce::dsp::IIR::Filter<float> airBassFilter[2];

    juce::SmoothedValue<float> inputGainSmoothed[2];
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> headroomSmoothed;

    float attackCoeff[2]  { 0.0f, 0.0f };
    float releaseCoeff[2] { 0.0f, 0.0f };

    int lastSidechainMode = -1;
    int lastAirBassMode = -1;

    std::array<TMTParams, 20> tmtChannels;

    float getParam(const juce::String& id) { return apvts.getRawParameterValue(id)->load(); }
    int getChoice(const juce::String& id) { return static_cast<int>(apvts.getRawParameterValue(id)->load()); }

    static float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IronCompressorAudioProcessor)
};
