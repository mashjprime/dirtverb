#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/ShimmerReverb.h"

class CinderProcessor : public juce::AudioProcessor
{
public:
    CinderProcessor();
    ~CinderProcessor() override;

    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Plugin info
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    // Programs (not used, but required)
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // State
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // Public state (for UI components)
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentReverbLevel{0.0f};
    std::atomic<float> outputRmsLevel{0.0f};
    std::atomic<float> outputPeakLevel{0.0f};

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP components
    ShimmerReverb shimmerReverbL, shimmerReverbR;

    // Parameter pointers (for fast access in processBlock)
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* shimmerParam = nullptr;
    std::atomic<float>* burnParam = nullptr;
    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* duckParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* freezeParam = nullptr;

    // Smoothed parameters to avoid zipper noise
    juce::SmoothedValue<float> driveSmoothed;
    juce::SmoothedValue<float> decaySmoothed;
    juce::SmoothedValue<float> shimmerSmoothed;
    juce::SmoothedValue<float> burnSmoothed;
    juce::SmoothedValue<float> sizeSmoothed;
    juce::SmoothedValue<float> duckSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> freezeSmoothed;

    // Envelope follower state (for sidechain ducking)
    float envState = 0.0f;
    float envAttackCoeff = 0.0f;   // ~0.5ms attack
    float envReleaseCoeff = 0.0f;  // ~150ms release

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CinderProcessor)
};
