#include "PluginProcessor.h"
#include "PluginEditor.h"

CinderProcessor::CinderProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for fast access
    decayParam = apvts.getRawParameterValue("decay");
    shimmerParam = apvts.getRawParameterValue("shimmer");
    degradeParam = apvts.getRawParameterValue("degrade");
    foldParam = apvts.getRawParameterValue("fold");
    dirtParam = apvts.getRawParameterValue("dirt");
    sizeParam = apvts.getRawParameterValue("size");
    mixParam = apvts.getRawParameterValue("mix");
    preParam = apvts.getRawParameterValue("pre");
    duckParam = apvts.getRawParameterValue("duck");
}

CinderProcessor::~CinderProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CinderProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // DECAY: 0.1s to 30s (with skew for better control at lower values)
    // At max value, treated as infinite
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"decay", 1},
        "Decay",
        juce::NormalisableRange<float>(0.1f, 30.0f, 0.01f, 0.3f),
        2.0f));

    // SHIMMER: how much pitch-shifted content in feedback (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"shimmer", 1},
        "Shimmer",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // DEGRADE: lo-fi amount (sample rate + bit reduction)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"degrade", 1},
        "Degrade",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // FOLD: wavefolder intensity
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fold", 1},
        "Fold",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // DIRT: blend between clean reverb and wavefolded reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"dirt", 1},
        "Dirt",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // SIZE: room size / diffusion density
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"size", 1},
        "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // MIX: dry/wet blend
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1},
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f));

    // PRE: pre/post destruction routing
    // 0 = post (destroy after reverb), 1 = pre (destroy before reverb)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pre", 1},
        "Pre",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // DUCK: sidechain ducking amount (dry envelope ducks wet signal)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"duck", 1},
        "Duck",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    return {params.begin(), params.end()};
}

void CinderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Initialize DSP components
    shimmerReverbL.prepare(sampleRate, samplesPerBlock);
    shimmerReverbR.prepare(sampleRate, samplesPerBlock);

    preLofiDegraderL.prepare(sampleRate);
    preLofiDegraderR.prepare(sampleRate);
    preWavefolderL.prepare(sampleRate);
    preWavefolderR.prepare(sampleRate);

    postLofiDegraderL.prepare(sampleRate);
    postLofiDegraderR.prepare(sampleRate);
    postWavefolderL.prepare(sampleRate);
    postWavefolderR.prepare(sampleRate);

    // Initialize smoothed parameters (50ms smoothing time)
    const double smoothingTime = 0.05;
    decaySmoothed.reset(sampleRate, smoothingTime);
    shimmerSmoothed.reset(sampleRate, smoothingTime);
    degradeSmoothed.reset(sampleRate, smoothingTime);
    foldSmoothed.reset(sampleRate, smoothingTime);
    dirtSmoothed.reset(sampleRate, smoothingTime);
    sizeSmoothed.reset(sampleRate, smoothingTime);
    mixSmoothed.reset(sampleRate, smoothingTime);
    preSmoothed.reset(sampleRate, smoothingTime);
    duckSmoothed.reset(sampleRate, smoothingTime);

    // Set initial values
    decaySmoothed.setCurrentAndTargetValue(*decayParam);
    shimmerSmoothed.setCurrentAndTargetValue(*shimmerParam);
    degradeSmoothed.setCurrentAndTargetValue(*degradeParam);
    foldSmoothed.setCurrentAndTargetValue(*foldParam);
    dirtSmoothed.setCurrentAndTargetValue(*dirtParam);
    sizeSmoothed.setCurrentAndTargetValue(*sizeParam);
    mixSmoothed.setCurrentAndTargetValue(*mixParam);
    preSmoothed.setCurrentAndTargetValue(*preParam);
    duckSmoothed.setCurrentAndTargetValue(*duckParam);

    // Envelope follower coefficients
    envAttackCoeff = std::exp(-1.0f / (0.0005f * static_cast<float>(sampleRate)));   // 0.5ms attack
    envReleaseCoeff = std::exp(-1.0f / (0.15f * static_cast<float>(sampleRate)));    // 150ms release
    envState = 0.0f;
}

void CinderProcessor::releaseResources()
{
    shimmerReverbL.reset();
    shimmerReverbR.reset();
    preLofiDegraderL.reset();
    preLofiDegraderR.reset();
    preWavefolderL.reset();
    preWavefolderR.reset();
    postLofiDegraderL.reset();
    postLofiDegraderR.reset();
    postWavefolderL.reset();
    postWavefolderR.reset();
    envState = 0.0f;
}

void CinderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Update smoothed parameter targets
    decaySmoothed.setTargetValue(*decayParam);
    shimmerSmoothed.setTargetValue(*shimmerParam);
    degradeSmoothed.setTargetValue(*degradeParam);
    foldSmoothed.setTargetValue(*foldParam);
    dirtSmoothed.setTargetValue(*dirtParam);
    sizeSmoothed.setTargetValue(*sizeParam);
    mixSmoothed.setTargetValue(*mixParam);
    preSmoothed.setTargetValue(*preParam);
    duckSmoothed.setTargetValue(*duckParam);

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : leftChannel;

    float peakLevel = 0.0f;
    float sumSquares = 0.0f;
    float blockPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Get smoothed parameter values
        const float decay = decaySmoothed.getNextValue();
        const float shimmer = shimmerSmoothed.getNextValue();
        const float degrade = degradeSmoothed.getNextValue();
        const float fold = foldSmoothed.getNextValue();
        const float dirt = dirtSmoothed.getNextValue();
        const float size = sizeSmoothed.getNextValue();
        const float mix = mixSmoothed.getNextValue();
        const float pre = preSmoothed.getNextValue();
        const float duck = duckSmoothed.getNextValue();

        // Check for infinite mode (decay > 29.5s treated as freeze)
        const bool infiniteMode = decay > 29.5f;
        const float actualDecay = infiniteMode ? 100.0f : decay;

        // Update reverb parameters
        shimmerReverbL.setParameters(actualDecay, shimmer, size);
        shimmerReverbR.setParameters(actualDecay, shimmer, size);

        // Update destruction parameters for both pre and post paths
        preLofiDegraderL.setDegrade(degrade);
        preLofiDegraderR.setDegrade(degrade);
        preWavefolderL.setFold(fold);
        preWavefolderR.setFold(fold);
        postLofiDegraderL.setDegrade(degrade);
        postLofiDegraderR.setDegrade(degrade);
        postWavefolderL.setFold(fold);
        postWavefolderR.setFold(fold);

        // 1. Save pristine dry input
        const float dryL = leftChannel[i];
        const float dryR = rightChannel[i];

        // 2. Envelope follower on dry signal (for sidechain ducking)
        const float dryMono = (std::abs(dryL) + std::abs(dryR)) * 0.5f;
        const float envCoeff = (dryMono > envState) ? envAttackCoeff : envReleaseCoeff;
        envState = envCoeff * envState + (1.0f - envCoeff) * dryMono;

        // 3. Pre-destruction path (destroy BEFORE reverb)
        float preDestroyedL = dryL;
        float preDestroyedR = dryR;
        if (pre > 0.001f)
        {
            float preDegL = preLofiDegraderL.process(dryL);
            float preDegR = preLofiDegraderR.process(dryR);
            float preFoldL = preWavefolderL.process(preDegL);
            float preFoldR = preWavefolderR.process(preDegR);
            preDestroyedL = preDegL * (1.0f - dirt) + preFoldL * dirt;
            preDestroyedR = preDegR * (1.0f - dirt) + preFoldR * dirt;
        }

        // 4. Reverb input: blend clean dry and pre-destroyed by PRE amount
        float reverbInL = dryL * (1.0f - pre) + preDestroyedL * pre;
        float reverbInR = dryR * (1.0f - pre) + preDestroyedR * pre;

        // 5. Shimmer reverb
        float reverbL = shimmerReverbL.process(reverbInL);
        float reverbR = shimmerReverbR.process(reverbInR);

        // 6. Post-destruction path (destroy AFTER reverb)
        float postDestroyedL = reverbL;
        float postDestroyedR = reverbR;
        if (pre < 0.999f)
        {
            float postDegL = postLofiDegraderL.process(reverbL);
            float postDegR = postLofiDegraderR.process(reverbR);
            float postFoldL = postWavefolderL.process(postDegL);
            float postFoldR = postWavefolderR.process(postDegR);
            postDestroyedL = postDegL * (1.0f - dirt) + postFoldL * dirt;
            postDestroyedR = postDegR * (1.0f - dirt) + postFoldR * dirt;
        }

        // 7. Wet signal: blend post-destroyed and clean reverb by PRE amount
        //    pre=0: fully post-destroyed (default, same as old behavior)
        //    pre=1: clean reverb (destruction happened before reverb)
        float wetL = postDestroyedL * (1.0f - pre) + reverbL * pre;
        float wetR = postDestroyedR * (1.0f - pre) + reverbR * pre;

        // 8. Apply sidechain ducking
        if (duck > 0.001f)
        {
            float duckGain = std::max(0.0f, 1.0f - duck * envState);
            wetL *= duckGain;
            wetR *= duckGain;
        }

        // 9. Final dry/wet mix
        leftChannel[i] = dryL * (1.0f - mix) + wetL * mix;
        rightChannel[i] = dryR * (1.0f - mix) + wetR * mix;

        // Track peak for visualization
        peakLevel = std::max(peakLevel, std::abs(wetL));

        // Accumulate for output metering
        float outSample = (leftChannel[i] + rightChannel[i]) * 0.5f;
        sumSquares += outSample * outSample;
        blockPeak = std::max(blockPeak, std::abs(outSample));
    }

    // Update visualization level
    currentReverbLevel.store(peakLevel);

    // Update output metering atomics
    if (numSamples > 0)
    {
        float rms = std::sqrt(sumSquares / static_cast<float>(numSamples));
        outputRmsLevel.store(rms, std::memory_order_relaxed);
        outputPeakLevel.store(blockPeak, std::memory_order_relaxed);
    }
}

void CinderProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CinderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorEditor* CinderProcessor::createEditor()
{
    return new CinderEditor(*this);
}

// Plugin instantiation
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CinderProcessor();
}
