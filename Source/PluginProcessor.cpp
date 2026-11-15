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

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("inputGain", "Input", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("outputGain", "Output", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("threshold", "Threshold", juce::NormalisableRange<float> (-40.0f, 0.0f, 0.1f), -10.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("biasL", "Bias L", juce::StringArray { "Low", "Mid", "High" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("biasR", "Bias R", juce::StringArray { "Low", "Mid", "High" }, 0));

    juce::StringArray rectifiers { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germanium 100nF" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("rectifier", "Rectifier", rectifiers, 2));

    juce::StringArray attacks { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("attack", "Attack", attacks, 2));

    juce::StringArray releases { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("release", "Release", releases, 2));

    juce::StringArray scEQs { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" };
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("sidechainEq", "Sidechain EQ", scEQs, 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("stereoWidth", "Width", juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("monoMaker", "Mono Maker", juce::NormalisableRange<float> (0.0f, 300.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> ("airBass", "Air/Bass", juce::StringArray { "Bypass", "AirBass", "Tape Roll-Off" }, 0));

    return { params.begin(), params.end() };
}

void IronCompressorAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    inputGainSmoothed.reset (sr, 0.02);
    outputGainSmoothed.reset (sr, 0.02);
    mixSmoothed.reset (sr, 0.02);
    envelope.fill (0.0f);
    lastSidechainMode = -1;
    lastAirBassMode = -1;
    updateTimingCoefficients();
}

void IronCompressorAudioProcessor::releaseResources() {}
bool IronCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo() && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void IronCompressorAudioProcessor::updateTimingCoefficients()
{
    static constexpr float attackMs[6]  { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f };
    static constexpr float releaseMs[6] { 200.0f, 300.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f };
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

void IronCompressorAudioProcessor::updateFilters(int scMode, int airBassMode)
{
    if (scMode != lastSidechainMode) {
        for (int ch = 0; ch < 2; ++ch) {
            if (scMode == 0) sidechainFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
            else if (scMode == 1) sidechainFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 200.0, 0.7f, 0.5f); 
            // Добавим заглушки для остальных, чтобы не было ошибок
            else sidechainFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
        }
        lastSidechainMode = scMode;
    }
    
    if (airBassMode != lastAirBassMode) {
        for (int ch = 0; ch < 2; ++ch) {
            if (airBassMode == 1) { // AirBass
                airBassFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 10000.0, 0.7f, 2.0f);
            } else if (airBassMode == 2) { // Tape
                airBassFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 15000.0);
            }
        }
        lastAirBassMode = airBassMode;
    }
}

void IronCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    if (numChannels < 2 || numSamples == 0) return;

    const float thresholdDb = apvts.getRawParameterValue("threshold")->load();
    const int biasIdxL      = apvts.getRawParameterValue("biasL")->load();
    const int biasIdxR      = apvts.getRawParameterValue("biasR")->load();
    const float mix         = apvts.getRawParameterValue("mix")->load();
    const float inGainDb    = apvts.getRawParameterValue("inputGain")->load();
    const float outGainDb   = apvts.getRawParameterValue("outputGain")->load();
    const float width       = apvts.getRawParameterValue("stereoWidth")->load();
    const float monoFreq    = apvts.getRawParameterValue("monoMaker")->load();
    const int scMode        = apvts.getRawParameterValue("sidechainEq")->load();
    const int airBassMode   = apvts.getRawParameterValue("airBass")->load();

    updateFilters(scMode, airBassMode);

    if (monoFreq > 20.0f) {
        auto monoCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, monoFreq);
        for (int ch = 0; ch < 2; ++ch) monoMakerFilter[ch].coefficients = monoCoeffs;
    }

    const float inGain  = dbToGain(inGainDb);
    const float outGain = dbToGain(outGainDb);

    inputGainSmoothed .setTargetValue(inGain);
    outputGainSmoothed.setTargetValue(outGain);
    mixSmoothed       .setTargetValue(mix);

    updateTimingCoefficients();

    const float biasMultL = (biasIdxL == 0) ? 0.5f : ((biasIdxL == 1) ? 1.0f : 2.0f);
    const float biasMultR = (biasIdxR == 0) ? 0.5f : ((biasIdxR == 1) ? 1.0f : 2.0f);
    
    float maxGRL = 0.0f;
    float maxGRR = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        float curInGain = inputGainSmoothed.getNextValue();
        float curOutGain = outputGainSmoothed.getNextValue();
        float curMix = mixSmoothed.getNextValue();

        float inL = buffer.getSample(0, n) * curInGain;
        float inR = buffer.getSample(1, n) * curInGain;

        float mid = (inL + inR) * 0.5f;
        float side = (inL - inR) * 0.5f;
        if (monoFreq > 20.0f) side = monoMakerFilter[0].processSample(side);
        side *= width;
        inL = mid + side;
        inR = mid - side;

        float scL = sidechainFilter[0].processSample(inL);
        float scR = sidechainFilter[1].processSample(inR);
        
        float envL = envelope[0];
        float envR = envelope[1];
        
        if (std::abs(scL) > envL) envL = attackCoeff * envL + (1.0f - attackCoeff) * std::abs(scL);
        else envL = releaseCoeff * envL + (1.0f - releaseCoeff) * std::abs(scL);
        
        if (std::abs(scR) > envR) envR = attackCoeff * envR + (1.0f - attackCoeff) * std::abs(scR);
        else envR = releaseCoeff * envR + (1.0f - releaseCoeff) * std::abs(scR);
        
        envelope[0] = envL;
        envelope[1] = envR;

        const float threshLin = dbToGain(thresholdDb);
        float cvL = (envL > threshLin) ? juce::jlimit(0.0f, 1.0f, ((envL / threshLin) - 1.0f) * biasMultL) : 0.0f;
        float cvR = (envR > threshLin) ? juce::jlimit(0.0f, 1.0f, ((envR / threshLin) - 1.0f) * biasMultR) : 0.0f;

        maxGRL = juce::jmax(maxGRL, cvL);
        maxGRR = juce::jmax(maxGRR, cvR);

        float dryL = buffer.getSample(0, n);
        float dryR = buffer.getSample(1, n);

        float wetL = std::tanh(inL * (1.0f - 0.6f * cvL));
        float wetR = std::tanh(inR * (1.0f - 0.6f * cvR));

        if (airBassMode > 0) {
            wetL = airBassFilter[0].processSample(wetL);
            wetR = airBassFilter[1].processSample(wetR);
        }

        float outL = (dryL * (1.0f - curMix)) + (wetL * curMix);
        float outR = (dryR * (1.0f - curMix)) + (wetR * curMix);

        buffer.setSample(0, n, outL * curOutGain);
        buffer.setSample(1, n, outR * curOutGain);
    }

    currentGRL.store(maxGRL);
    currentGRR.store(maxGRR);
}

juce::AudioProcessorEditor* IronCompressorAudioProcessor::createEditor() { return new IronCompressorAudioProcessorEditor (*this); }
void IronCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}
void IronCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new IronCompressorAudioProcessor(); }
