#include "PluginProcessor.h"
#include "PluginEditor.h"

CinderProcessor::CinderProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for fast access
    driveParam = apvts.getRawParameterValue("drive");
    decayParam = apvts.getRawParameterValue("decay");
    shimmerParam = apvts.getRawParameterValue("shimmer");
    burnParam = apvts.getRawParameterValue("burn");
    sizeParam = apvts.getRawParameterValue("size");
    duckParam = apvts.getRawParameterValue("duck");
    mixParam = apvts.getRawParameterValue("mix");
}

CinderProcessor::~CinderProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CinderProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // DRIVE: input saturation — how hard the signal hits the reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drive", 1},
        "Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

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

    // BURN: saturation inside FDN feedback loop — progressive distortion per echo
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"burn", 1},
        "Burn",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // SIZE: room size / diffusion density
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"size", 1},
        "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // DUCK: sidechain ducking amount (dry envelope ducks wet signal)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"duck", 1},
        "Duck",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    // MIX: dry/wet blend
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1},
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.3f));

    return {params.begin(), params.end()};
}

void CinderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Initialize DSP components
    shimmerReverbL.prepare(sampleRate, samplesPerBlock);
    shimmerReverbR.prepare(sampleRate, samplesPerBlock);

    // Initialize smoothed parameters (50ms smoothing time)
    const double smoothingTime = 0.05;
    driveSmoothed.reset(sampleRate, smoothingTime);
    decaySmoothed.reset(sampleRate, smoothingTime);
    shimmerSmoothed.reset(sampleRate, smoothingTime);
    burnSmoothed.reset(sampleRate, smoothingTime);
    sizeSmoothed.reset(sampleRate, smoothingTime);
    duckSmoothed.reset(sampleRate, smoothingTime);
    mixSmoothed.reset(sampleRate, smoothingTime);

    // Set initial values
    driveSmoothed.setCurrentAndTargetValue(*driveParam);
    decaySmoothed.setCurrentAndTargetValue(*decayParam);
    shimmerSmoothed.setCurrentAndTargetValue(*shimmerParam);
    burnSmoothed.setCurrentAndTargetValue(*burnParam);
    sizeSmoothed.setCurrentAndTargetValue(*sizeParam);
    duckSmoothed.setCurrentAndTargetValue(*duckParam);
    mixSmoothed.setCurrentAndTargetValue(*mixParam);

    // Envelope follower coefficients
    envAttackCoeff = std::exp(-1.0f / (0.0005f * static_cast<float>(sampleRate)));   // 0.5ms attack
    envReleaseCoeff = std::exp(-1.0f / (0.15f * static_cast<float>(sampleRate)));    // 150ms release
    envState = 0.0f;
}

void CinderProcessor::releaseResources()
{
    shimmerReverbL.reset();
    shimmerReverbR.reset();
    envState = 0.0f;
}

void CinderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Update smoothed parameter targets
    driveSmoothed.setTargetValue(*driveParam);
    decaySmoothed.setTargetValue(*decayParam);
    shimmerSmoothed.setTargetValue(*shimmerParam);
    burnSmoothed.setTargetValue(*burnParam);
    sizeSmoothed.setTargetValue(*sizeParam);
    duckSmoothed.setTargetValue(*duckParam);
    mixSmoothed.setTargetValue(*mixParam);

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : leftChannel;

    float peakLevel = 0.0f;
    float sumSquares = 0.0f;
    float blockPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Get smoothed parameter values
        const float drive = driveSmoothed.getNextValue();
        const float decay = decaySmoothed.getNextValue();
        const float shimmer = shimmerSmoothed.getNextValue();
        const float burn = burnSmoothed.getNextValue();
        const float size = sizeSmoothed.getNextValue();
        const float duck = duckSmoothed.getNextValue();
        const float mix = mixSmoothed.getNextValue();

        // Check for infinite mode (decay > 29.5s treated as freeze)
        const bool infiniteMode = decay > 29.5f;
        const float actualDecay = infiniteMode ? 100.0f : decay;

        // 1. Save pristine dry input
        const float dryL = leftChannel[i];
        const float dryR = rightChannel[i];

        // 2. Envelope follower on dry signal (for sidechain ducking)
        const float dryMono = (std::abs(dryL) + std::abs(dryR)) * 0.5f;
        const float envCoeff = (dryMono > envState) ? envAttackCoeff : envReleaseCoeff;
        envState = envCoeff * envState + (1.0f - envCoeff) * dryMono;

        // 3. Apply DRIVE saturation (warm input distortion)
        //    driveGain: 1x (clean) to 6x (heavy saturation)
        float driveGain = 1.0f + drive * 5.0f;
        float drivenL = std::tanh(dryL * driveGain);
        float drivenR = std::tanh(dryR * driveGain);

        // 4. Update reverb parameters (burn is applied inside the feedback loop)
        shimmerReverbL.setParameters(actualDecay, shimmer, size, burn);
        shimmerReverbR.setParameters(actualDecay, shimmer, size, burn);

        // 5. Process shimmer reverb
        float wetL = shimmerReverbL.process(drivenL);
        float wetR = shimmerReverbR.process(drivenR);

        // 6. Apply sidechain ducking
        //    envState is raw amplitude (0-1 range for typical signals).
        //    Scale by 5x so a signal peaking at ~0.5 drives full ducking.
        if (duck > 0.001f)
        {
            float envScaled = std::min(envState * 5.0f, 1.0f);
            float duckGain = std::max(0.0f, 1.0f - duck * envScaled);
            wetL *= duckGain;
            wetR *= duckGain;
        }

        // 7. Final dry/wet mix
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
