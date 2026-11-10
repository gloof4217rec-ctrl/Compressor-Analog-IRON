#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
IronCompressorAudioProcessor::IronCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

IronCompressorAudioProcessor::~IronCompressorAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
IronCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Threshold [-60 .. 0] dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "threshold", "Threshold", juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f));

    // Ratio [1 .. 20]
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "ratio", "Ratio", juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f), 4.0f));

    // Rectifier choice (6 positions)
    juce::StringArray rectifiers { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF",
                                   "Silicon 330nF", "Germanium 220nF", "Silicon/Germanium 100nF" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "rectifier", "Rectifier", rectifiers, 2)); // default: LED

    // Attack choice (6 steps)
    juce::StringArray attacks { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "attack", "Attack", attacks, 2)); // default: 0.5 ms

    // Release choice (6 steps)
    juce::StringArray releases { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "release", "Release", releases, 2)); // default: 500 ms

    // Tube bias [0 .. 1]
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bias", "Tube Bias", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    // Dry/Wet mix
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));

    // Input / Output gain [-12 .. +12] dB
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "inputGain", "Input Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outputGain", "Output Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));

    // Sidechain HPF on/off (fixed 20 Hz, as in original)
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "sidechainHPF", "Sidechain HPF", true));

    return { params.begin(), params.end() };
}

//==============================================================================
void IronCompressorAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    sr = sampleRate;

    // 20 Hz sidechain high‑pass (always in circuit in the original)
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 20.0);
    for (int ch = 0; ch < 2; ++ch)
        sidechainHPF[ch].coefficients = hpCoeffs;

    inputGainSmoothed .reset (sr, 0.02);
    outputGainSmoothed.reset (sr, 0.02);
    mixSmoothed       .reset (sr, 0.02);

    envelope.fill (0.0f);
    updateTimingCoefficients();
}

void IronCompressorAudioProcessor::releaseResources() {}

bool IronCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void IronCompressorAudioProcessor::updateTimingCoefficients()
{
    // Base times from the selected attack / release steps
    static constexpr float attackMs[6]  { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f };
    static constexpr float releaseMs[6] { 200.0f, 300.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f };

    // Rectifier "character" scales the base times.
    // Germanium → slower, Silicon → faster, LED → medium, etc.
    static constexpr float rectAttackFactor[6]  { 1.5f, 1.2f, 1.0f, 0.8f, 0.6f, 0.4f };
    static constexpr float rectReleaseFactor[6] { 1.8f, 1.5f, 1.0f, 0.7f, 0.5f, 0.3f };

    const int attIdx = apvts.getRawParameterValue ("attack")->load();
    const int relIdx = apvts.getRawParameterValue ("release")->load();
    const int recIdx = apvts.getRawParameterValue ("rectifier")->load();

    const float attackSec  = (attackMs[attIdx]  * rectAttackFactor[recIdx])  / 1000.0f;
    const float releaseSec = (releaseMs[relIdx] * rectReleaseFactor[recIdx]) / 1000.0f;

    attackCoeff  = std::exp (-1.0f / (attackSec  * (float) sr));
    releaseCoeff = std::exp (-1.0f / (releaseSec * (float) sr));
}

//==============================================================================
void IronCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    if (numChannels < 2 || numSamples == 0)
        return;

    // --- read parameters ---
    const float thresholdDb = apvts.getRawParameterValue ("threshold")->load();
    const float ratio       = apvts.getRawParameterValue ("ratio")->load();
    const float biasAmount  = apvts.getRawParameterValue ("bias")->load();
    const float mix         = apvts.getRawParameterValue ("mix")->load();
    const float inGainDb    = apvts.getRawParameterValue ("inputGain")->load();
    const float outGainDb   = apvts.getRawParameterValue ("outputGain")->load();
    const bool  hpfOn       = apvts.getRawParameterValue ("sidechainHPF")->load() > 0.5f;

    const float inGain  = dbToGain (inGainDb);
    const float outGain = dbToGain (outGainDb);

    inputGainSmoothed .setTargetValue (inGain);
    outputGainSmoothed.setTargetValue (outGain);
    mixSmoothed       .setTargetValue (mix);

    // Timing coefficients are cheap to recompute; do it every block when
    // the relevant parameters change (here we just recompute unconditionally
    // for simplicity – in a real‑time‑critical path you may cache last values).
    updateTimingCoefficients();

    // -----------------------------------------------------------------
    // Audio processing (per sample, per channel)
    // -----------------------------------------------------------------
    for (int n = 0; n < numSamples; ++n)
    {
        // smoothed gains / mix
        const float curInGain  = inputGainSmoothed .getNextValue();
        const float curOutGain = outputGainSmoothed.getNextValue();
        const float curMix     = mixSmoothed       .getNextValue();

        // -----------------------------------------------------------------
        // 1) Sidechain envelope detection (stereo‑linked: use max of both
        //    channels, as in the original when Link is engaged).
        // -----------------------------------------------------------------
        float sidechainSample = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            float s = buffer.getSample (ch, n) * curInGain;
            if (hpfOn)
                s = sidechainHPF[ch].processSample (s);
            sidechainSample = juce::jmax (sidechainSample, std::abs (s));
        }

        // -----------------------------------------------------------------
        // 2) Envelope follower with attack / release coefficients.
        // -----------------------------------------------------------------
        float& env = envelope[0]; // single envelope for both channels (link)
        if (sidechainSample > env)
            env = attackCoeff  * env + (1.0f - attackCoeff)  * sidechainSample;
        else
            env = releaseCoeff * env + (1.0f - releaseCoeff) * sidechainSample;

        // -----------------------------------------------------------------
        // 3) Compute control voltage (CV) from threshold, ratio and bias.
        // -----------------------------------------------------------------
        const float threshLin = dbToGain (thresholdDb);
        float cv = 0.0f;
        if (env > threshLin)
        {
            const float over = env / juce::jmax (threshLin, 1e-6f); // linear over‑threshold
            // Convert to a "gain reduction" style CV.
            // Simplified: CV increases with over‑threshold and ratio.
            cv = (over - 1.0f) * (ratio - 1.0f) * 0.5f;
            cv = juce::jlimit (0.0f, 1.0f, cv);
        }

        // Bias modulates the operating point of the tubes.
        const float biasMod = cv * biasAmount;

        // -----------------------------------------------------------------
        // 4) Parallel tube stage.
        //    Tube A (12AU7, remote cut‑off): soft saturation, gain depends
        //    on bias. Tube B (12AX7, sharp cut‑off): harder knee.
        // -----------------------------------------------------------------
        for (int ch = 0; ch < 2; ++ch)
        {
            const float dry = buffer.getSample (ch, n);

            float x = dry * curInGain;

            // Tube A – remote cut‑off (12AU7)
            // As biasMod increases, the effective gain of this tube is reduced.
            const float tubeAGain = 1.0f - 0.6f * biasMod;
            float tubeA = std::tanh (x * tubeAGain);

            // Tube B – sharp cut‑off (12AX7)
            // Harder knee, independent of bias (bias applied to grids simultaneously,
            // but the sharp cut‑off character is modelled with a higher drive and
            // asymmetrical clipping).
            float tubeB = std::tanh (x * 1.8f + 0.2f * biasMod);
            tubeB *= 0.7f; // balance the two parallel paths

            // Recombine the two tube paths (parallel topology)
            float wet = 0.6f * tubeA + 0.4f * tubeB;

            // Optional: subtle output transformer saturation (Iron transformers)
            wet = std::tanh (wet * 1.1f);

            // Apply output gain and dry/wet mix
            wet *= curOutGain;
            const float out = dry * (1.0f - curMix) + wet * curMix;

            buffer.setSample (ch, n, out);
        }
    }
}

//==============================================================================
juce::AudioProcessorEditor* IronCompressorAudioProcessor::createEditor()
{
    return new IronCompressorAudioProcessorEditor (*this);
}

//==============================================================================
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