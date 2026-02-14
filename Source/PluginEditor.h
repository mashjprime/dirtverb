#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/CinderLookAndFeel.h"
#include "UI/WaveformVisualizer.h"
#include "UI/OutputMeter.h"

// --- Reusable knob widget ---

class CinderKnob : public juce::Slider
{
public:
    CinderKnob()
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setPopupDisplayEnabled(true, true, nullptr);
        setVelocityBasedMode(true);
        setVelocityModeParameters(1.0, 1, 0.0, true);
        setDoubleClickReturnValue(true, 0.0);
    }
};

// --- Content panel (draws at design dimensions, scaled by parent) ---

class CinderContentPanel : public juce::Component
{
public:
    // Section title Y positions (set by resized in editor)
    std::array<int, 3> sectionYPositions {};
    CinderLookAndFeel* laf = nullptr;

    void paint(juce::Graphics& g) override;
};

// --- Main Editor ---

class CinderEditor : public juce::AudioProcessorEditor
{
public:
    CinderEditor(CinderProcessor&);
    ~CinderEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int designW = 520;
    static constexpr int designH = 440;

    CinderProcessor& processor;
    CinderLookAndFeel cinderLook;
    juce::TooltipWindow tooltipWindow { this, 400 };

    // Scaled content panel
    CinderContentPanel contentPanel;
    juce::ComponentBoundsConstrainer constrainer;

    // Visualizer + meter (initialized in constructor with processor refs)
    WaveformVisualizer waveformVisualizer;
    OutputMeter outputMeter;

    // Knobs
    CinderKnob decayKnob, shimmerKnob, sizeKnob;
    CinderKnob degradeKnob, foldKnob, dirtKnob;
    CinderKnob mixKnob;

    // Labels
    juce::Label decayLabel, shimmerLabel, sizeLabel;
    juce::Label degradeLabel, foldLabel, dirtLabel;
    juce::Label mixLabel;

    // Parameter attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAtt> decayAtt, shimmerAtt, sizeAtt;
    std::unique_ptr<SliderAtt> degradeAtt, foldAtt, dirtAtt;
    std::unique_ptr<SliderAtt> mixAtt;

    void setupLabel(juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CinderEditor)
};
