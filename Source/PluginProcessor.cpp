#include "PluginProcessor.h"
#include "PluginEditor.h"

IronCompressorAudioProcessor::IronCompressorAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    juce::Random r;
    for (auto& ch : tmtChannels) {
        ch.threshOffset = r.nextFloat() * 0.6f - 0.3f;
        ch.timeOffset   = 0.95f + (r.nextFloat() * 0.1f);
        ch.biasOffset   = 0.98f + (r.nextFloat() * 0.04f);
    }

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
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

    addFloat("mix", "Mix", 0.0f, 1.0f, 1.0f, 0.01f);
    addFloat("headroom", "Headroom", -6.0f, 6.0f, 0.0f);
    addFloat("stereoWidth", "Width", 0.0f, 2.0f, 1.0f, 0.01f);
    addFloat("monoMaker", "Mono Maker", 0.0f, 300.0f, 0.0f, 1.0f);
    
    addBool("link", "Link L->R", false);
    addBool("midSide", "M/S Mode", false);
    addBool("tmtEnable", "TMT Enable", true);
    addBool("tmtStereoMode", "TMT Analog", false);
    params.push_back(std::make_unique<juce::AudioParameterInt>("tmtChannel", "TMT Channel", 0, 9, 3));

    addFloat("thresholdL", "Threshold L", -40.0f, 0.0f, -10.0f);
    addFloat("inputGainL", "Input L", -12.0f, 12.0f, 0.0f);
    addFloat("outputGainL", "Output L", -12.0f, 12.0f, 0.0f);
    addChoice("biasL", "Bias L", { "Low", "Mid", "High" }, 0);
    addChoice("rectifierL", "Rectifier L", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" }, 2);
    addChoice("attackL", "Attack L", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" }, 2);
    addChoice("releaseL", "Release L", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" }, 2);
    addChoice("airBassL", "Air/Tape L", { "Bypass", "AirBass", "Tape Roll-Off" }, 0);
    addChoice("sidechainEqL", "SC EQ L", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" }, 0);

    addFloat("thresholdR", "Threshold R", -40.0f, 0.0f, -10.0f);
    addFloat("inputGainR", "Input R", -12.0f, 12.0f, 0.0f);
    addFloat("outputGainR", "Output R", -12.0f, 12.0f, 0.0f);
    addChoice("biasR", "Bias R", { "Low", "Mid", "High" }, 0);
    addChoice("rectifierR", "Rectifier R", { "Germanium 1mF", "Germanium 2mF", "LED 3.3mF", "Silicon 330nF", "Germanium 220nF", "Silicon/Germ" }, 2);
    addChoice("attackR", "Attack R", { "0.1 ms", "0.2 ms", "0.5 ms", "1 ms", "2 ms", "5 ms" }, 2);
    addChoice("releaseR", "Release R", { "200 ms", "300 ms", "500 ms", "1 s", "2 s", "5 s" }, 2);
    addChoice("airBassR", "Air/Tape R", { "Bypass", "AirBass", "Tape Roll-Off" }, 0);
    addChoice("sidechainEqR", "SC EQ R", { "Off", "Preset 1", "Preset 2", "Preset 3", "Preset 4" }, 0);

    return { params.begin(), params.end() };
}

void IronCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (int ch = 0; ch < 2; ++ch) {
        inputGainSmoothed[ch].reset(sr, 0.02);
        outputGainSmoothed[ch].reset(sr, 0.02);
        envelope[ch] = 0.0f;
        optoState[ch] = 0.0f;
    }
    mixSmoothed.reset(sr, 0.02);
    headroomSmoothed.reset(sr, 0.02);

    oversampler->initProcessing(samplesPerBlock);
    oversampler->reset();

    lastSidechainModeL = -1;
    lastSidechainModeR = -1;
    lastAirBassModeL = -1;
    lastAirBassModeR = -1;
}

void IronCompressorAudioProcessor::releaseResources() {}
bool IronCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void IronCompressorAudioProcessor::updateTimingCoefficients(int attL, int relL, int recL, int attR, int relR, int recR, double currentSr)
{
    static constexpr float attackTable[6][6] = {
        { 0.1f, 6.0f, 10.0f, 18.0f, 30.0f, 50.0f },
        { 1.0f, 15.0f, 30.0f, 40.0f, 50.0f, 70.0f },
        { 3.0f, 35.0f, 70.0f, 100.0f, 150.0f, 220.0f },
        { 0.5f, 3.0f, 5.0f, 8.0f, 9.0f, 12.0f },
        { 0.3f, 1.5f, 3.0f, 5.0f, 7.0f, 9.0f },
        { 0.2f, 0.7f, 1.5f, 2.5f, 4.0f, 6.0f }
    };

    static constexpr float releaseTable[6][6] = {
        { 100.0f, 150.0f, 180.0f, 200.0f, 220.0f, 250.0f },
        { 300.0f, 450.0f, 500.0f, 600.0f, 700.0f, 900.0f },
        { 600.0f, 1000.0f, 1700.0f, 2500.0f, 3200.0f, 5000.0f },
        { 80.0f, 120.0f, 160.0f, 180.0f, 220.0f, 300.0f },
        { 30.0f, 50.0f, 70.0f, 80.0f, 120.0f, 130.0f },
        { 20.0f, 40.0f, 60.0f, 80.0f, 100.0f, 170.0f }
    };

    float aSecL = attackTable[recL][attL] / 1000.0f;
    float rSecL = releaseTable[recL][relL] / 1000.0f;
    attackCoeff[0] = std::exp(-1.0f / (juce::jmax(aSecL, 0.0001f) * (float)currentSr));
    releaseCoeff[0] = std::exp(-1.0f / (juce::jmax(rSecL, 0.0001f) * (float)currentSr));

    float aSecR = attackTable[recR][attR] / 1000.0f;
    float rSecR = releaseTable[recR][relR] / 1000.0f;
    attackCoeff[1] = std::exp(-1.0f / (juce::jmax(aSecR, 0.0001f) * (float)currentSr));
    releaseCoeff[1] = std::exp(-1.0f / (juce::jmax(rSecR, 0.0001f) * (float)currentSr));
}

void IronCompressorAudioProcessor::updateFilters()
{
    int scModeL = getChoice("sidechainEqL");
    int scModeR = getChoice("sidechainEqR");
    int airBassL = getChoice("airBassL");
    int airBassR = getChoice("airBassR");
    float monoFreq = getParam("monoMaker");

    if (scModeL != lastSidechainModeL) {
        if (scModeL == 0) sidechainFilter[0].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
        else if (scModeL == 1) sidechainFilter[0].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 200.0, 0.7f, 0.5f);
        else sidechainFilter[0].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
        lastSidechainModeL = scModeL;
    }
    if (scModeR != lastSidechainModeR) {
        if (scModeR == 0) sidechainFilter[1].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
        else if (scModeR == 1) sidechainFilter[1].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sr, 200.0, 0.7f, 0.5f);
        else sidechainFilter[1].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sr, 20.0);
        lastSidechainModeR = scModeR;
    }

    if (airBassL != lastAirBassModeL) {
        if (airBassL == 1) airBassFilter[0].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 10000.0, 0.7f, 2.0f);
        else if (airBassL == 2) airBassFilter[0].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 15000.0);
        lastAirBassModeL = airBassL;
    }
    if (airBassR != lastAirBassModeR) {
        if (airBassR == 1) airBassFilter[1].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sr, 10000.0, 0.7f, 2.0f);
        else if (airBassR == 2) airBassFilter[1].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 15000.0);
        lastAirBassModeR = airBassR;
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

    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampler->processSamplesUp(block);

    const bool isLink = getParam("link") > 0.5f;
    const bool isMidSide = getParam("midSide") > 0.5f;
    const bool isTMTEnabled = getParam("tmtEnable") > 0.5f;

    const float headroom_dB = getParam("headroom");
    const float mix = getParam("mix");
    const float width = getParam("stereoWidth");
    const float monoFreq = getParam("monoMaker");

    const bool tmtDigital = getParam("tmtStereoMode") > 0.5f;
    const int tmtCh = static_cast<int>(getParam("tmtChannel"));
    int tmtIdxL = tmtCh * 2;
    int tmtIdxR = tmtDigital ? tmtCh * 2 : tmtCh * 2 + 1;

    float threshOffsetL = isTMTEnabled ? tmtChannels[tmtIdxL].threshOffset : 0.0f;
    float timeOffsetL   = isTMTEnabled ? tmtChannels[tmtIdxL].timeOffset   : 1.0f;
    float biasOffsetL   = isTMTEnabled ? tmtChannels[tmtIdxL].biasOffset   : 1.0f;

    float threshOffsetR = isTMTEnabled ? tmtChannels[tmtIdxR].threshOffset : 0.0f;
    float timeOffsetR   = isTMTEnabled ? tmtChannels[tmtIdxR].timeOffset   : 1.0f;
    float biasOffsetR   = isTMTEnabled ? tmtChannels[tmtIdxR].biasOffset   : 1.0f;

    float threshL_dB = getParam("thresholdL") + threshOffsetL;
    float inGainL = dbToGain(getParam("inputGainL"));
    float outGainL = dbToGain(getParam("outputGainL"));
    const int biasL = getChoice("biasL");
    const int attackL = getChoice("attackL");
    const int releaseL = getChoice("releaseL");
    const int rectifierL = getChoice("rectifierL");

    float threshR_dB = isLink ? threshL_dB : (getParam("thresholdR") + threshOffsetR);
    float inGainR = isLink ? inGainL : dbToGain(getParam("inputGainR"));
    float outGainR = isLink ? outGainL : dbToGain(getParam("outputGainR"));
    const int biasR = isLink ? biasL : getChoice("biasR");
    const int attackR = isLink ? attackL : getChoice("attackR");
    const int releaseR = isLink ? releaseL : getChoice("releaseR");
    const int rectifierR = isLink ? rectifierL : getChoice("rectifierR");

    updateFilters();
    
    double osSr = sr * 4.0; 
    updateTimingCoefficients(attackL, releaseL, rectifierL, attackR, releaseR, rectifierR, osSr);

    inputGainSmoothed[0].setTargetValue(inGainL);
    inputGainSmoothed[1].setTargetValue(inGainR);
    outputGainSmoothed[0].setTargetValue(outGainL);
    outputGainSmoothed[1].setTargetValue(outGainR);
    mixSmoothed.setTargetValue(mix);
    headroomSmoothed.setTargetValue(headroom_dB);

    const float biasMultL = ((biasL == 0) ? 0.5f : ((biasL == 1) ? 1.0f : 2.0f)) * biasOffsetL;
    const float biasMultR = ((biasR == 0) ? 0.5f : ((biasR == 1) ? 1.0f : 2.0f)) * biasOffsetR;

    float maxGR_dB_L = 0.0f;
    float maxGR_dB_R = 0.0f;
    float maxOutL = 0.0f;
    float maxOutR = 0.0f;

    float optoTimeSec = 0.01f; 
    float optoCoeff = 1.0f - std::exp(-1.0f / (optoTimeSec * (float)osSr));

    int osNumSamples = oversampledBlock.getNumSamples();
    for (int n = 0; n < osNumSamples; ++n)
    {
        const float curHeadroom = headroomSmoothed.getNextValue();
        const float curMix = mixSmoothed.getNextValue();
        
        float rawL = oversampledBlock.getSample(0, n);
        float rawR = oversampledBlock.getSample(1, n);

        float mid = (rawL + rawR) * 0.5f;
        float side = (rawL - rawR) * 0.5f;

        float procL, procR;

        if (isMidSide) {
            procL = mid * inputGainSmoothed[0].getNextValue();
            procR = side * inputGainSmoothed[1].getNextValue();
        } else {
            procL = rawL * inputGainSmoothed[0].getNextValue();
            procR = rawR * inputGainSmoothed[1].getNextValue();
        }

        float detectL = procL * dbToGain(-curHeadroom);
        float detectR = procR * dbToGain(-curHeadroom);

        float scL = sidechainFilter[0].processSample(detectL);
        float scR = sidechainFilter[1].processSample(detectR);

        if (isLink) {
            float maxSc = juce::jmax(std::abs(scL), std::abs(scR));
            scL = maxSc;
            scR = maxSc;
        }

        float envL = envelope[0];
        float envR = envelope[1];
        
        float aCoeffL = 1.0f - (1.0f - attackCoeff[0]) * timeOffsetL;
        float rCoeffL = 1.0f - (1.0f - releaseCoeff[0]) * timeOffsetL;
        float aCoeffR = 1.0f - (1.0f - attackCoeff[1]) * timeOffsetR;
        float rCoeffR = 1.0f - (1.0f - releaseCoeff[1]) * timeOffsetR;

        if (std::abs(scL) > envL) envL = aCoeffL * envL + (1.0f - aCoeffL) * std::abs(scL);
        else envL = rCoeffL * envL + (1.0f - rCoeffL) * std::abs(scL);

        if (std::abs(scR) > envR) envR = aCoeffR * envR + (1.0f - aCoeffR) * std::abs(scR);
        else envR = rCoeffR * envR + (1.0f - rCoeffR) * std::abs(scR);

        envelope[0] = envL;
        envelope[1] = envR;

        const float threshLinL = dbToGain(threshL_dB);
        const float threshLinR = dbToGain(threshR_dB);

        float cvL = (envL > threshLinL) ? juce::jlimit(0.0f, 1.0f, ((envL / threshLinL) - 1.0f) * biasMultL) : 0.0f;
        float cvR = (envR > threshLinR) ? juce::jlimit(0.0f, 1.0f, ((envR / threshLinR) - 1.0f) * biasMultR) : 0.0f;

        optoState[0] += (cvL - optoState[0]) * optoCoeff;
        optoState[1] += (cvR - optoState[1]) * optoCoeff;
        
        float finalCvL = juce::jmax(cvL, optoState[0]); 
        float finalCvR = juce::jmax(cvR, optoState[1]);

        if (finalCvL > 0.001f) maxGR_dB_L = juce::jmin(maxGR_dB_L, juce::Decibels::gainToDecibels(1.0f - finalCvL, -100.0f));
        if (finalCvR > 0.001f) maxGR_dB_R = juce::jmin(maxGR_dB_R, juce::Decibels::gainToDecibels(1.0f - finalCvR, -100.0f));

        float wetL = std::tanh(procL * (1.0f - 0.6f * finalCvL));
        float wetR = std::tanh(procR * (1.0f - 0.6f * finalCvR));

        if (getChoice("airBassL") > 0) wetL = airBassFilter[0].processSample(wetL);
        if (getChoice("airBassR") > 0) wetR = airBassFilter[1].processSample(wetR);

        float outL = (procL * (1.0f - curMix)) + (wetL * curMix);
        float outR = (procR * (1.0f - curMix)) + (wetR * curMix);

        if (isMidSide) {
            float outMid = outL * outputGainSmoothed[0].getNextValue();
            float outSide = outR * outputGainSmoothed[1].getNextValue();
            outL = outMid + outSide;
            outR = outMid - outSide;
        } else {
            outL *= outputGainSmoothed[0].getNextValue();
            outR *= outputGainSmoothed[1].getNextValue();
        }

        float finalMid = (outL + outR) * 0.5f;
        float finalSide = (outL - outR) * 0.5f;

        if (monoFreq > 20.0f) finalSide = monoMakerFilter[0].processSample(finalSide);
        finalSide *= width;

        outL = finalMid + finalSide;
        outR = finalMid - finalSide;

        // 👇 ИСПРАВЛЕНИЕ: Финальный Soft-Clipper, чтобы убрать цифровой шум при +12 дБ 👇
        outL = std::tanh(outL);
        outR = std::tanh(outR);

        maxOutL = juce::jmax(maxOutL, std::abs(outL));
        maxOutR = juce::jmax(maxOutR, std::abs(outR));

        oversampledBlock.setSample(0, n, outL);
        oversampledBlock.setSample(1, n, outR);
    }

    oversampler->processSamplesDown(block);

    currentGR_dB_L.store(maxGR_dB_L);
    currentGR_dB_R.store(maxGR_dB_R);
    currentOut_dB_L.store(juce::Decibels::gainToDecibels(maxOutL, -100.0f));
    currentOut_dB_R.store(juce::Decibels::gainToDecibels(maxOutR, -100.0f));
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
