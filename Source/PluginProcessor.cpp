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

    // 👇 ИСПРАВЛЕНИЕ: Правильный расчет коэффициента опто-лимитера (10 мс)
    float optoTimeSec = 0.01f; 
    float optoCoeff = 1.0f - std::exp(-1.0f / (optoTimeSec * (float)osSr));

    int osNumSamples = oversampledBlock.getNumSamples();
    for (int n = 0; n < osNumSamples; ++n)
    {
        const float curHeadroom = headroomSmoothed.getNextValue();
        const float curMix = mixSmoothed.getNextValue();
        
        float rawL = oversampledBlock.getSample(0, n);
        float rawR = oversampledBlock.getSample(1, n);

        // 1. Кодируем в Mid/Side (если включено)
        float mid = (rawL + rawR) * 0.5f;
        float side = (rawL - rawR) * 0.5f;

        float procL, procR;

        // 2. Применяем Input Gain к Mid/Side или L/R
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

        // 👇 ИСПРАВЛЕНО: Используем правильный optoCoeff 👇
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
