#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/DirtLookAndFeel.h"
#include "UI/WaveformVisualizer.h"

class DirtverbEditor : public juce::AudioProcessorEditor,
                       public juce::Timer
{
public:
    DirtverbEditor(DirtverbProcessor&);
    ~DirtverbEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    DirtverbProcessor& processor;
    DirtLookAndFeel dirtLookAndFeel;

    // Knobs
    juce::Slider decayKnob, shimmerKnob, degradeKnob, foldKnob;
    juce::Slider dirtKnob, sizeKnob, mixKnob;

    // Labels
    juce::Label decayLabel, shimmerLabel, degradeLabel, foldLabel;
    juce::Label dirtLabel, sizeLabel, mixLabel;
    juce::Label titleLabel;
    juce::Label infiniteIndicator;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shimmerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> degradeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> foldAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dirtAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    // Visualizer
    WaveformVisualizer waveformVisualizer;

    // Helper to set up a knob
    void setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirtverbEditor)
};
