#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/ShimmerReverb.h"
#include "DSP/LofiDegrader.h"
#include "DSP/Wavefolder.h"

class DirtverbProcessor : public juce::AudioProcessor
{
public:
    DirtverbProcessor();
    ~DirtverbProcessor() override;

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

    // Parameter access
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    // For visualization
    float getCurrentReverbLevel() const { return currentReverbLevel.load(); }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP components
    ShimmerReverb shimmerReverbL, shimmerReverbR;
    LofiDegrader lofiDegraderL, lofiDegraderR;
    Wavefolder wavefolderL, wavefolderR;

    // Parameter pointers (for fast access in processBlock)
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* shimmerParam = nullptr;
    std::atomic<float>* degradeParam = nullptr;
    std::atomic<float>* foldParam = nullptr;
    std::atomic<float>* dirtParam = nullptr;
    std::atomic<float>* sizeParam = nullptr;
    std::atomic<float>* mixParam = nullptr;

    // Smoothed parameters to avoid zipper noise
    juce::SmoothedValue<float> decaySmoothed;
    juce::SmoothedValue<float> shimmerSmoothed;
    juce::SmoothedValue<float> degradeSmoothed;
    juce::SmoothedValue<float> foldSmoothed;
    juce::SmoothedValue<float> dirtSmoothed;
    juce::SmoothedValue<float> sizeSmoothed;
    juce::SmoothedValue<float> mixSmoothed;

    // For visualization
    std::atomic<float> currentReverbLevel{0.0f};

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirtverbProcessor)
};
