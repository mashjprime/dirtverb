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
    // Section title Y positions (3 sections: REVERB, FIRE, OUTPUT)
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

    // Knobs — REVERB section
    CinderKnob decayKnob, shimmerKnob, sizeKnob;
    // Knobs — FIRE section
    CinderKnob driveKnob, burnKnob;
    // Knobs — OUTPUT section
    CinderKnob duckKnob, mixKnob;

    // Labels
    juce::Label decayLabel, shimmerLabel, sizeLabel;
    juce::Label driveLabel, burnLabel;
    juce::Label duckLabel, mixLabel;

    // Parameter attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAtt> decayAtt, shimmerAtt, sizeAtt;
    std::unique_ptr<SliderAtt> driveAtt, burnAtt;
    std::unique_ptr<SliderAtt> duckAtt, mixAtt;

    void setupLabel(juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CinderEditor)
};
