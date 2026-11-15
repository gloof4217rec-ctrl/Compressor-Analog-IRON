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

    // TMT Параметры
    addBool("tmtEnable", "TMT Enable", true); // НОВЫЙ ПАРАМЕТР ВКЛ/ВЫКЛ
    addBool("tmtStereoMode", "TMT Stereo", true); 
    params.push_back(std::make_unique<juce::AudioParameterInt>("tmtChannel", "TMT Channel", 0, 9, 7));

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

void IronCompressorAudioProcessor::updateTimingCoefficients(int attL, int relL, int recL, int attR, int relR, int recR)
{
    static constexpr float attackMs[6]  { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f };
    static constexpr float releaseMs[6] { 200.0f, 300.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f };
    static constexpr float rectAttackFactor[6]  { 1.3f, 1.1f, 1.5f, 0.6f, 1.0f, 0.8f };
    static constexpr float rectReleaseFactor[6] { 1.4f, 1.2f, 1.8f, 0.5f, 0.9f, 0.7f };

    const float attackSecL  = (attackMs[attL]  * rectAttackFactor[recL])  / 1000.0f;
    const float releaseSecL = (releaseMs[relL] * rectReleaseFactor[recL]) / 1000.0f;
    attackCoeff[0]  = std::exp (-1.0f / (juce::jmax(attackSecL, 0.0001f)  * (float) sr));
    releaseCoeff[0] = std::exp (-1.0f / (juce::jmax(releaseSecL, 0.0001f) * (float) sr));

    const float attackSecR  = (attackMs[attR]  * rectAttackFactor[recR])  / 1000.0f;
    const float releaseSecR = (releaseMs[relR] * rectReleaseFactor[recR]) / 1000.0f;
    attackCoeff[1]  = std::exp (-1.0f / (juce::jmax(attackSecR, 0.0001f)  * (float) sr));
    releaseCoeff[1] = std::exp (-1.0f / (juce::jmax(releaseSecR, 0.0001f) * (float) sr));
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
    
    // Проверяем, включен ли TMT
    const bool isTMTEnabled = getParam("tmtEnable") > 0.5f;

    const float headroom_dB = getParam("headroom");
    const float mix = getParam("mix");
    const float outGain = dbToGain(getParam("outputGain"));
    const float width = getParam("stereoWidth");
    const float monoFreq = getParam("monoMaker");

    const bool tmtDigital = getParam("tmtStereoMode") > 0.5f;
    const int tmtCh = static_cast<int>(getParam("tmtChannel"));
    
    int tmtIdxL = tmtCh * 2;
    int tmtIdxR = tmtDigital ? tmtCh * 2 : tmtCh * 2 + 1;

    // Если TMT выключен, обнуляем индексы (сдвигаем на нулевой канал без отклонений)
    // Но лучше просто игнорировать отклонения в математике ниже.
    
    // Получаем отклонения (или нули, если TMT выключен)
    float threshOffsetL = isTMTEnabled ? tmtChannels[tmtIdxL].threshOffset : 0.0f;
    float timeOffsetL   = isTMTEnabled ? tmtChannels[tmtIdxL].timeOffset   : 1.0f;
    float biasOffsetL   = isTMTEnabled ? tmtChannels[tmtIdxL].biasOffset   : 1.0f;

    float threshOffsetR = isTMTEnabled ? tmtChannels[tmtIdxR].threshOffset : 0.0f;
    float timeOffsetR   = isTMTEnabled ? tmtChannels[tmtIdxR].timeOffset   : 1.0f;
    float biasOffsetR   = isTMTEnabled ? tmtChannels[tmtIdxR].biasOffset   : 1.0f;

    float threshL_dB = getParam("thresholdL") + threshOffsetL;
    float inGainL = dbToGain(getParam("inputGainL"));
    const int biasL = getChoice("biasL");
    const int attackL = getChoice("attackL");
    const int releaseL = getChoice("releaseL");
    const int rectifierL = getChoice("rectifierL");

    float threshR_dB = isLink ? threshL_dB : (getParam("thresholdR") + threshOffsetR);
    float inGainR = isLink ? inGainL : dbToGain(getParam("inputGainR"));
    const int biasR = isLink ? biasL : getChoice("biasR");
    const int attackR = isLink ? attackL : getChoice("attackR");
    const int releaseR = isLink ? releaseL : getChoice("releaseR");
    const int rectifierR = isLink ? rectifierL : getChoice("rectifierR");

    updateFilters();
    updateTimingCoefficients(attackL, releaseL, rectifierL, attackR, releaseR, rectifierR);

    inputGainSmoothed[0].setTargetValue(inGainL);
    inputGainSmoothed[1].setTargetValue(inGainR);
    outputGainSmoothed.setTargetValue(outGain);
    mixSmoothed.setTargetValue(mix);
    headroomSmoothed.setTargetValue(headroom_dB);

    const float biasMultL = ((biasL == 0) ? 0.5f : ((biasL == 1) ? 1.0f : 2.0f)) * biasOffsetL;
    const float biasMultR = ((biasR == 0) ? 0.5f : ((biasR == 1) ? 1.0f : 2.0f)) * biasOffsetR;

    float maxGR_dB_L = 0.0f;
    float maxGR_dB_R = 0.0f;

    for (int n = 0; n < numSamples; ++n)
    {
        const float curHeadroom = headroomSmoothed.getNextValue();
        const float curMix = mixSmoothed.getNextValue();
        const float curOutGain = outputGainSmoothed.getNextValue();

        float inL = buffer.getSample(0, n) * inputGainSmoothed[0].getNextValue();
        float inR = buffer.getSample(1, n) * inputGainSmoothed[1].getNextValue();

        float leftSignal = inL;
        float rightSignal = inR;

        if (isMidSide) {
            leftSignal = (inL + inR) * 0.5f;
            rightSignal = (inL - inR) * 0.5f;
        }

        float detectL = leftSignal * dbToGain(-curHeadroom);
        float detectR = rightSignal * dbToGain(-curHeadroom);

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

        if (cvL > 0.001f) {
            float currentGR = juce::Decibels::gainToDecibels(1.0f - cvL, -100.0f);
            maxGR_dB_L = juce::jmin(maxGR_dB_L, currentGR);
        }
        if (cvR > 0.001f) {
            float currentGR = juce::Decibels::gainToDecibels(1.0f - cvR, -100.0f);
            maxGR_dB_R = juce::jmin(maxGR_dB_R, currentGR);
        }

        float wetL = std::tanh(leftSignal * (1.0f - 0.6f * cvL));
        float wetR = std::tanh(rightSignal * (1.0f - 0.6f * cvR));

        if (getChoice("airBass") > 0) {
            wetL = airBassFilter[0].processSample(wetL);
            wetR = airBassFilter[1].processSample(wetR);
        }

        float outL = (leftSignal * (1.0f - curMix)) + (wetL * curMix);
        float outR = (rightSignal * (1.0f - curMix)) + (wetR * curMix);

        if (isMidSide) {
            float tempL = outL + outR;
            float tempR = outL - outR;
            outL = tempL;
            outR = tempR;
        }

        float mid = (outL + outR) * 0.5f;
        float side = (outL - outR) * 0.5f;

        if (monoFreq > 20.0f) {
            side = monoMakerFilter[0].processSample(side);
        }
        side *= width;

        outL = mid + side;
        outR = mid - side;

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
