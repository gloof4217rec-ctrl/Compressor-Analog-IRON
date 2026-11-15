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

    auto addFloat = [&](const juce::String& id, const juce::String& name, float min, float max, float def, float step = 0.1f) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, juce::NormalisableRange<float>(min, max, step), def));
    };
    auto addChoice = [&](const juce::String& id, const juce::String& name, const juce::StringArray& items, int def) {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(id, name, items, def));
    };
    auto addBool = [&](const juce::String& id, const juce::String& name, bool def) {
        params.push_back(std::make_unique<juce::AudioParameterBool>(id, name, def));
    };

    addFloat("outputGain", "Output", -12.0f, 12.0f, 0.0f);
    addFloat("mix", "Mix", 0.0f, 1.0f, 1.0f, 0.01f);
    addFloat("headroom", "Headroom", -6.0f, 6.0f, 0.0f);
    addFloat("stereoWidth", "Width", 0.0f, 2.0f, 1.0f, 0.01f);
    addFloat("monoMaker", "Mono Maker", 0.0f, 300.0f, 0.0f, 1.0f);
    addChoice("sidechainEq", "SC EQ", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" }, 0);
    addChoice("airBass", "Air/Tape", { "Bypass", "AirBass", "Tape Roll-Off" }, 0);

    addBool("link", "Link L->R", false);
    addBool("midSide", "M/S Mode", false);

    addFloat("thresholdL", "Threshold L", -40.0f, 0.0f, -10.0f);
    addFloat("inputGainL", "Input L", -12.0f, 12.0f, 0.0f);
    addChoice("biasL", "Bias L", { "Low", "Mid", "High" }, 0);
    addChoice("rectifierL", "Rectifier L", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" }, 2);
    addChoice("attackL", "Attack L", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" }, 2);
    addChoice("releaseL", "Release L", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" }, 2);

    addFloat("thresholdR", "Threshold R", -40.0f, 0.0f, -10.0f);
    addFloat("inputGainR", "Input R", -12.0f, 12.0f, 0.0f);
    addChoice("biasR", "Bias R", { "Low", "Mid", "High" }, 0);
    addChoice("rectifierR", "Rectifier R", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" }, 2);
    addChoice("attackR", "Attack R", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" }, 2);
    addChoice("releaseR", "Release R", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" }, 2);

    return { params.begin(), params.end() };
}

void IronCompressorAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (int ch = 0; ch < 2; ++ch) {
        inputGainSmoothed[ch].reset(sr, 0.02);
        envelope[ch] = 0.0f;
    }
    outputGainSmoothed.reset(sr, 0.02);
    mixSmoothed.reset(sr, 0.02);
    headroomSmoothed.reset(sr, 0.02);

    lastSidechainMode = -1;
    lastAirBassMode = -1;
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
    static constexpr float rectAttackFactor[6]  { 1.3f, 1.1f, 1.5f, 0.6f, 1.0f, 0.8f };
    static constexpr float rectReleaseFactor[6] { 1.4f, 1.2f, 1.8f, 0.5f, 0.9f, 0.7f };

    for (int ch = 0; ch < 2; ++ch) {
        const int attIdx = getChoice(ch == 0 ? "attackL" : "attackR");
        const int relIdx = getChoice(ch == 0 ? "releaseL" : "releaseR");
        const int recIdx = getChoice(ch == 0 ? "rectifierL" : "rectifierR");

        const float attackSec  = (attackMs[attIdx]  * rectAttackFactor[recIdx])  / 1000.0f;
        const float releaseSec = (releaseMs[relIdx] * rectReleaseFactor[recIdx]) / 1000.0f;

        attackCoeff[ch]  = std::exp (-1.0f / (juce::jmax(attackSec, 0.0001f)  * (float) sr));
        releaseCoeff[ch] = std::exp (-1.0f / (juce::jmax(releaseSec, 0.0001f) * (float) sr));
    }
}

void IronCompressorAudioProcessor::updateFilters()
{
    const int scMode = getChoice("sidechainEq");
    const int airBassMode = getChoice("airBass");
    const float monoFreq = getParam("monoMaker");

    if (scMode != lastSidechainMode) {
        for (int ch = 0; ch < 2; ++ch) {
            if (scMode == 0) sidechainFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
            else if (scMode == 1) sidechainFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 200.0, 0.7f, 0.5f);
            else sidechainFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
        }
        lastSidechainMode = scMode;
    }

    if (airBassMode != lastAirBassMode) {
        for (int ch = 0; ch < 2; ++ch) {
            if (airBassMode == 1) {
                airBassFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 10000.0, 0.7f, 2.0f);
            } else if (airBassMode == 2) {
                airBassFilter[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 15000.0);
            }
        }
        lastAirBassMode = airBassMode;
    }

    if (monoFreq > 20.0f) {
        auto monoCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, monoFreq);
        for (int ch = 0; ch < 2; ++ch) monoMakerFilter[ch].coefficients = monoCoeffs;
    }
}

void IronCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples == 0) return;

    const bool isLink = getParam("link") > 0.5f;
    const bool isMidSide = getParam("midSide") > 0.5f;

    const float headroom_dB = getParam("headroom");
    const float mix = getParam("mix");
    const float outGain = dbToGain(getParam("outputGain"));
    const float width = getParam("stereoWidth");
    const float monoFreq = getParam("monoMaker");

    float threshL_dB = getParam("thresholdL");
    float inGainL = dbToGain(getParam("inputGainL"));
    const int biasL = getChoice("biasL");

    float threshR_dB = isLink ? threshL_dB : getParam("thresholdR");
    float inGainR = isLink ? inGainL : dbToGain(getParam("inputGainR"));
    const int biasR = isLink ? biasL : getChoice("biasR");

    updateFilters();
    updateTimingCoefficients();

    inputGainSmoothed[0].setTargetValue(inGainL);
    inputGainSmoothed[1].setTargetValue(inGainR);
    outputGainSmoothed.setTargetValue(outGain);
    mixSmoothed.setTargetValue(mix);
    headroomSmoothed.setTargetValue(headroom_dB);

    const float biasMultL = (biasL == 0) ? 0.5f : ((biasL == 1) ? 1.0f : 2.0f);
    const float biasMultR = (biasR == 0) ? 0.5f : ((biasR == 1) ? 1.0f : 2.0f);

    float maxGR_dB_L = 0.0f;
    float maxGR_dB_R = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float curHeadroom = headroomSmoothed.getNextValue();
        const float curMix = mixSmoothed.getNextValue();
        const float curOutGain = outputGainSmoothed.getNextValue();

        float inL = buffer.getSample(0, n) * inputGainSmoothed[0].getNextValue();
        float inR = buffer.getSample(1, n) * inputGainSmoothed[1].getNextValue();

        float detectL = inL * dbToGain(curHeadroom);
        float detectR = inR * dbToGain(curHeadroom);

        if (isMidSide) {
            const float mid = (detectL + detectR) * 0.5f;
            const float side = (detectL - detectR) * 0.5f;
            detectL = mid;
            detectR = side;
        }

        const float scL = sidechainFilter[0].processSample(detectL);
        const float scR = sidechainFilter[1].processSample(detectR);

        float envL = envelope[0];
        float envR = envelope[1];

        if (std::abs(scL) > envL) envL = attackCoeff[0] * envL + (1.0f - attackCoeff[0]) * std::abs(scL);
        else envL = releaseCoeff[0] * envL + (1.0f - releaseCoeff[0]) * std::abs(scL);

        if (std::abs(scR) > envR) envR = attackCoeff[1] * envR + (1.0f - attackCoeff[1]) * std::abs(scR);
        else envR = releaseCoeff[1] * envR + (1.0f - releaseCoeff[1]) * std::abs(scR);

        envelope[0] = envL;
        envelope[1] = envR;

        const float threshLinL = dbToGain(threshL_dB);
        const float threshLinR = dbToGain(threshR_dB);

        float cvL = (envL > threshLinL) ? juce::jlimit(0.0f, 1.0f, ((envL / threshLinL) - 1.0f) * biasMultL) : 0.0f;
        float cvR = (envR > threshLinR) ? juce::jlimit(0.0f, 1.0f, ((envR / threshLinR) - 1.0f) * biasMultR) : 0.0f;

        if (cvL > 0.001f) {
            float currentGR = juce::Decibels::gainToDecibels(1.0f - cvL, -100.0f);
            maxGR_dB_L = juce::jmin(maxGR_dB_L, currentGR);
        }
        if (cvR > 0.001f) {
            float currentGR = juce::Decibels::gainToDecibels(1.0f - cvR, -100.0f);
            maxGR_dB_R = juce::jmin(maxGR_dB_R, currentGR);
        }

        float wetL = std::tanh(inL * (1.0f - 0.6f * cvL));
        float wetR = std::tanh(inR * (1.0f - 0.6f * cvR));

        if (getChoice("airBass") > 0) {
            wetL = airBassFilter[0].processSample(wetL);
            wetR = airBassFilter[1].processSample(wetR);
        }

        float outL = (inL * (1.0f - curMix)) + (wetL * curMix);
        float outR = (inR * (1.0f - curMix)) + (wetR * curMix);

        if (monoFreq > 20.0f) {
            const float mid = (outL + outR) * 0.5f;
            float side = (outL - outR) * 0.5f;
            side = monoMakerFilter[0].processSample(side);
            side *= width;
            outL = mid + side;
            outR = mid - side;
        } else {
            const float mid = (outL + outR) * 0.5f;
            float side = (outL - outR) * 0.5f;
            side *= width;
            outL = mid + side;
            outR = mid - side;
        }

        buffer.setSample(0, n, outL * curOutGain);
        buffer.setSample(1, n, outR * curOutGain);
    }

    currentGR_dB_L.store(maxGR_dB_L);
    currentGR_dB_R.store(maxGR_dB_R);
}

juce::AudioProcessorEditor* IronCompressorAudioProcessor::createEditor() { return new IronCompressorAudioProcessorEditor (*this); }

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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new IronCompressorAudioProcessor(); }
