#include "PluginProcessor.h"
#include "PluginEditor.h"

IronCompressorAudioProcessor::IronCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

IronCompressorAudioProcessor::~IronCompressorAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout
IronCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input / Output Gain
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "inputGain", "Input", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    // Threshold
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "threshold", "Threshold", juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f), -10.0f));

    // Tube Bias (Low, Mid, High)
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "bias", "Tube Bias", juce::StringArray { "Low", "Mid", "High" }, 0));

    // Rectifier (Влияет на скорость)
    juce::StringArray rectifiers { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF",
                                   "Silicon 330nF", "Germanium 220nF", "Silicon/Germanium 100nF" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "rectifier", "Rectifier", rectifiers, 2)); // По умолчанию LED

    // Attack / Release
    juce::StringArray attacks { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "attack", "Attack", attacks, 2));

    juce::StringArray releases { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "release", "Release", releases, 2));

    // Sidechain EQ (Off, Presets, Ext)
    juce::StringArray scEQs { "Off (20Hz HPF)", "Preset 1", "Preset 2", "Preset 3", "Preset 4", "External" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "sidechainEq", "Sidechain EQ", scEQs, 0));

    // Parallel Mix
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

void IronCompressorAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;
    
    // Инициализация фильтров
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 20.0);
    for (int ch = 0; ch < 2; ++ch) {
        sidechainHPF[ch].coefficients = hpCoeffs;
        sidechainPreset[ch].coefficients = hpCoeffs; // Заглушка, обновится в processBlock
    }

    inputGainSmoothed .reset (sr, 0.02);
    outputGainSmoothed.reset (sr, 0.02);
    mixSmoothed       .reset (sr, 0.02);

    envelope.fill (0.0f);
    lastSidechainMode = -1;
    updateTimingCoefficients();
}

void IronCompressorAudioProcessor::releaseResources() {}

bool IronCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void IronCompressorAudioProcessor::updateTimingCoefficients()
{
    static constexpr float attackMs[6]  { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f };
    static constexpr float releaseMs[6] { 200.0f, 300.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f };

    // LED (индекс 2) - самый медленный, Silicon (индекс 3) - самый быстрый
    static constexpr float rectAttackFactor[6]  { 1.3f, 1.1f, 1.5f, 0.6f, 1.0f, 0.8f };
    static constexpr float rectReleaseFactor[6] { 1.4f, 1.2f, 1.8f, 0.5f, 0.9f, 0.7f };

    const int attIdx = apvts.getRawParameterValue ("attack")->load();
    const int relIdx = apvts.getRawParameterValue ("release")->load();
    const int recIdx = apvts.getRawParameterValue ("rectifier")->load();

    const float attackSec  = (attackMs[attIdx]  * rectAttackFactor[recIdx])  / 1000.0f;
    const float releaseSec = (releaseMs[relIdx] * rectReleaseFactor[recIdx]) / 1000.0f;

    attackCoeff  = std::exp (-1.0f / (attackSec  * (float) sr));
    releaseCoeff = std::exp (-1.0f / (releaseSec * (float) sr));
}

void IronCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    if (numChannels < 2 || numSamples == 0) return;

    const float thresholdDb = apvts.getRawParameterValue ("threshold")->load();
    const int biasIdx       = apvts.getRawParameterValue ("bias")->load();
    const float mix         = apvts.getRawParameterValue ("mix")->load();
    const float inGainDb    = apvts.getRawParameterValue ("inputGain")->load();
    const float outGainDb   = apvts.getRawParameterValue ("outputGain")->load();
    
    const int scMode = apvts.getRawParameterValue ("sidechainEq")->load();
    
    // Обновляем фильтры Sidechain, если режим изменился
    if (scMode != lastSidechainMode) {
        for (int ch = 0; ch < 2; ++ch) {
            if (scMode == 0) { // Off (20Hz HPF)
                sidechainPreset[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
            } else if (scMode == 1) { // Preset 1 (High shelf boost для меньшего сжатия баса)
                sidechainPreset[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 200.0, 0.7f, 2.0f);
            } else if (scMode == 2) { // Preset 2
                sidechainPreset[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 100.0);
            } else {
                sidechainPreset[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
            }
        }
        lastSidechainMode = scMode;
    }

    const float inGain  = dbToGain (inGainDb);
    const float outGain = dbToGain (outGainDb);

    inputGainSmoothed .setTargetValue (inGain);
    outputGainSmoothed.setTargetValue (outGain);
    mixSmoothed       .setTargetValue (mix);

    updateTimingCoefficients();

    // Bias Multiplier (Low = 0.5, Mid = 1.0, High = 2.0) - влияет на силу сжатия
    const float biasMult = (biasIdx == 0) ? 0.5f : ((biasIdx == 1) ? 1.0f : 2.0f);

    for (int n = 0; n < numSamples; ++n)
    {
        const float curInGain  = inputGainSmoothed .getNextValue();
        const float curOutGain = outputGainSmoothed.getNextValue();
        const float curMix     = mixSmoothed       .getNextValue();

        // Детектор огибающей
        float sidechainSample = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            float s = buffer.getSample (ch, n) * curInGain;
            s = sidechainPreset[ch].processSample (s); // Применяем Sidechain EQ
            sidechainSample = juce::jmax (sidechainSample, std::abs (s));
        }

        float& env = envelope[0];
        if (sidechainSample > env)
            env = attackCoeff  * env + (1.0f - attackCoeff)  * sidechainSample;
        else
            env = releaseCoeff * env + (1.0f - releaseCoeff) * sidechainSample;

        // Вычисление Control Voltage (управляющего напряжения)
        const float threshLin = dbToGain (thresholdDb);
        float cv = 0.0f;
        if (env > threshLin)
        {
            const float over = env / juce::jmax (threshLin, 1e-6f);
            cv = (over - 1.0f) * biasMult; // Убрали Ratio, теперь сила зависит от Bias
            cv = juce::jlimit (0.0f, 1.0f, cv);
        }

        for (int ch = 0; ch < 2; ++ch)
        {
            const float dry = buffer.getSample (ch, n);
            float x = dry * curInGain;

            // Параллельные лампы
            const float tubeAGain = 1.0f - 0.6f * cv; // 12AU7 (Remote cutoff)
            float tubeA = std::tanh (x * tubeAGain);

            float tubeB = std::tanh (x * 1.8f + 0.2f * cv); // 12AX7 (Sharp cutoff)
            tubeB *= 0.7f;

            float wet = 0.6f * tubeA + 0.4f * tubeB;
            wet = std::tanh (wet * 1.1f); // Трансформатор

            wet *= curOutGain;
            const float out = dry * (1.0f - curMix) + wet * curMix;

            buffer.setSample (ch, n, out);
        }
    }
}

juce::AudioProcessorEditor* IronCompressorAudioProcessor::createEditor()
{
    return new IronCompressorAudioProcessorEditor (*this);
}

void IronCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void IronCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IronCompressorAudioProcessor();
}
