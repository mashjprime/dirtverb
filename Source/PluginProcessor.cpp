#include "PluginProcessor.h"
#include "PluginEditor.h"

DirtverbProcessor::DirtverbProcessor()
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
}

DirtverbProcessor::~DirtverbProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout DirtverbProcessor::createParameterLayout()
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

    return {params.begin(), params.end()};
}

void DirtverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Initialize DSP components
    shimmerReverbL.prepare(sampleRate, samplesPerBlock);
    shimmerReverbR.prepare(sampleRate, samplesPerBlock);
    lofiDegraderL.prepare(sampleRate);
    lofiDegraderR.prepare(sampleRate);
    wavefolderL.prepare(sampleRate);
    wavefolderR.prepare(sampleRate);

    // Initialize smoothed parameters (50ms smoothing time)
    const double smoothingTime = 0.05;
    decaySmoothed.reset(sampleRate, smoothingTime);
    shimmerSmoothed.reset(sampleRate, smoothingTime);
    degradeSmoothed.reset(sampleRate, smoothingTime);
    foldSmoothed.reset(sampleRate, smoothingTime);
    dirtSmoothed.reset(sampleRate, smoothingTime);
    sizeSmoothed.reset(sampleRate, smoothingTime);
    mixSmoothed.reset(sampleRate, smoothingTime);

    // Set initial values
    decaySmoothed.setCurrentAndTargetValue(*decayParam);
    shimmerSmoothed.setCurrentAndTargetValue(*shimmerParam);
    degradeSmoothed.setCurrentAndTargetValue(*degradeParam);
    foldSmoothed.setCurrentAndTargetValue(*foldParam);
    dirtSmoothed.setCurrentAndTargetValue(*dirtParam);
    sizeSmoothed.setCurrentAndTargetValue(*sizeParam);
    mixSmoothed.setCurrentAndTargetValue(*mixParam);
}

void DirtverbProcessor::releaseResources()
{
    shimmerReverbL.reset();
    shimmerReverbR.reset();
    lofiDegraderL.reset();
    lofiDegraderR.reset();
}

void DirtverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : leftChannel;

    float peakLevel = 0.0f;

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

        // Check for infinite mode (decay > 29.5s treated as freeze)
        const bool infiniteMode = decay > 29.5f;
        const float actualDecay = infiniteMode ? 100.0f : decay;

        // Update DSP parameters
        shimmerReverbL.setParameters(actualDecay, shimmer, size);
        shimmerReverbR.setParameters(actualDecay, shimmer, size);
        lofiDegraderL.setDegrade(degrade);
        lofiDegraderR.setDegrade(degrade);
        wavefolderL.setFold(fold);
        wavefolderR.setFold(fold);

        // Get input samples
        const float dryL = leftChannel[i];
        const float dryR = rightChannel[i];

        // 1. Process through shimmer reverb
        float reverbL = shimmerReverbL.process(dryL);
        float reverbR = shimmerReverbR.process(dryR);

        // 2. Apply lo-fi degradation
        float degradedL = lofiDegraderL.process(reverbL);
        float degradedR = lofiDegraderR.process(reverbR);

        // 3. Parallel paths: clean vs wavefolded
        float cleanL = degradedL;
        float cleanR = degradedR;
        float foldedL = wavefolderL.process(degradedL);
        float foldedR = wavefolderR.process(degradedR);

        // 4. Blend clean and folded (DIRT parameter)
        float wetL = cleanL * (1.0f - dirt) + foldedL * dirt;
        float wetR = cleanR * (1.0f - dirt) + foldedR * dirt;

        // 5. Final dry/wet mix
        leftChannel[i] = dryL * (1.0f - mix) + wetL * mix;
        rightChannel[i] = dryR * (1.0f - mix) + wetR * mix;

        // Track peak for visualization
        peakLevel = std::max(peakLevel, std::abs(wetL));
    }

    // Update visualization level (with smoothing)
    currentReverbLevel.store(peakLevel);
}

void DirtverbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DirtverbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorEditor* DirtverbProcessor::createEditor()
{
    return new DirtverbEditor(*this);
}

// Plugin instantiation
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DirtverbProcessor();
}
