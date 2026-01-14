#include "PluginEditor.h"

DirtverbEditor::DirtverbEditor(DirtverbProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Apply custom look and feel
    setLookAndFeel(&dirtLookAndFeel);

    // Set up all knobs
    setupKnob(decayKnob, decayLabel, "DECAY");
    setupKnob(shimmerKnob, shimmerLabel, "SHIMMER");
    setupKnob(degradeKnob, degradeLabel, "DEGRADE");
    setupKnob(foldKnob, foldLabel, "FOLD");
    setupKnob(dirtKnob, dirtLabel, "DIRT");
    setupKnob(sizeKnob, sizeLabel, "SIZE");
    setupKnob(mixKnob, mixLabel, "MIX");

    // Create parameter attachments
    auto& apvts = processor.getAPVTS();
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "decay", decayKnob);
    shimmerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "shimmer", shimmerKnob);
    degradeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "degrade", degradeKnob);
    foldAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "fold", foldKnob);
    dirtAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "dirt", dirtKnob);
    sizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "size", sizeKnob);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "mix", mixKnob);

    // Title label
    titleLabel.setText("DIRTVERB", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("Consolas", 28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, dirtLookAndFeel.accent);
    addAndMakeVisible(titleLabel);

    // Infinite indicator (shows when decay is at max)
    infiniteIndicator.setText(juce::CharPointer_UTF8("\xe2\x88\x9e"), juce::dontSendNotification);
    infiniteIndicator.setFont(juce::Font("Consolas", 24.0f, juce::Font::bold));
    infiniteIndicator.setJustificationType(juce::Justification::centred);
    infiniteIndicator.setColour(juce::Label::textColourId, dirtLookAndFeel.shimmerColor);
    infiniteIndicator.setAlpha(0.0f);
    addAndMakeVisible(infiniteIndicator);

    // Visualizer
    addAndMakeVisible(waveformVisualizer);

    // Start timer for UI updates
    startTimerHz(30);

    // Set window size (experimental aspect ratio)
    setSize(520, 400);
}

DirtverbEditor::~DirtverbEditor()
{
    setLookAndFeel(nullptr);
}

void DirtverbEditor::setupKnob(juce::Slider& knob, juce::Label& label, const juce::String& name)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    knob.setName(name);
    addAndMakeVisible(knob);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void DirtverbEditor::paint(juce::Graphics& g)
{
    // Background gradient
    juce::ColourGradient bgGradient(
        dirtLookAndFeel.backgroundDark, 0.0f, 0.0f,
        dirtLookAndFeel.backgroundMid, 0.0f, static_cast<float>(getHeight()),
        false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Subtle noise texture overlay
    juce::Random rng(42); // Fixed seed for consistent pattern
    g.setColour(juce::Colours::white.withAlpha(0.02f));
    for (int i = 0; i < 500; ++i)
    {
        float x = rng.nextFloat() * static_cast<float>(getWidth());
        float y = rng.nextFloat() * static_cast<float>(getHeight());
        g.fillRect(x, y, 1.0f, 1.0f);
    }

    // Separator lines
    g.setColour(dirtLookAndFeel.knobTrack);
    
    // Line under title
    g.drawHorizontalLine(55, 20.0f, static_cast<float>(getWidth() - 20));
    
    // Line between top and bottom rows
    g.drawHorizontalLine(210, 20.0f, static_cast<float>(getWidth() - 20));

    // Section labels
    g.setColour(dirtLookAndFeel.textColor.withAlpha(0.5f));
    g.setFont(juce::Font("Consolas", 10.0f, juce::Font::plain));
    g.drawText("REVERB", 20, 60, 80, 15, juce::Justification::left);
    g.drawText("DESTRUCTION", 20, 215, 120, 15, juce::Justification::left);
}

void DirtverbEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title area
    titleLabel.setBounds(bounds.getX(), 15, bounds.getWidth(), 35);
    infiniteIndicator.setBounds(bounds.getWidth() - 50, 15, 40, 35);

    // Knob dimensions
    const int knobSize = 80;
    const int labelHeight = 20;
    const int knobSpacing = 10;
    
    // Top row: DECAY, SHIMMER, SIZE (reverb controls)
    // Starting at y = 75
    const int topRowY = 80;
    const int row1StartX = 30;
    
    decayKnob.setBounds(row1StartX, topRowY, knobSize, knobSize);
    decayLabel.setBounds(row1StartX, topRowY + knobSize, knobSize, labelHeight);
    
    shimmerKnob.setBounds(row1StartX + knobSize + knobSpacing, topRowY, knobSize, knobSize);
    shimmerLabel.setBounds(row1StartX + knobSize + knobSpacing, topRowY + knobSize, knobSize, labelHeight);
    
    sizeKnob.setBounds(row1StartX + (knobSize + knobSpacing) * 2, topRowY, knobSize, knobSize);
    sizeLabel.setBounds(row1StartX + (knobSize + knobSpacing) * 2, topRowY + knobSize, knobSize, labelHeight);

    // Waveform visualizer (right side of top row)
    const int vizX = row1StartX + (knobSize + knobSpacing) * 3 + 20;
    waveformVisualizer.setBounds(vizX, topRowY, bounds.getWidth() - vizX - 20, knobSize + labelHeight);

    // Bottom row: DEGRADE, FOLD, DIRT, MIX (destruction controls)
    const int bottomRowY = 235;
    
    degradeKnob.setBounds(row1StartX, bottomRowY, knobSize, knobSize);
    degradeLabel.setBounds(row1StartX, bottomRowY + knobSize, knobSize, labelHeight);
    
    foldKnob.setBounds(row1StartX + knobSize + knobSpacing, bottomRowY, knobSize, knobSize);
    foldLabel.setBounds(row1StartX + knobSize + knobSpacing, bottomRowY + knobSize, knobSize, labelHeight);
    
    dirtKnob.setBounds(row1StartX + (knobSize + knobSpacing) * 2, bottomRowY, knobSize, knobSize);
    dirtLabel.setBounds(row1StartX + (knobSize + knobSpacing) * 2, bottomRowY + knobSize, knobSize, labelHeight);
    
    // MIX knob larger and on the right
    const int mixKnobSize = knobSize + 20;
    mixKnob.setBounds(bounds.getWidth() - mixKnobSize - 30, bottomRowY - 10, mixKnobSize, mixKnobSize);
    mixLabel.setBounds(bounds.getWidth() - mixKnobSize - 30, bottomRowY + mixKnobSize - 10, mixKnobSize, labelHeight);
}

void DirtverbEditor::timerCallback()
{
    // Update visualizer with current levels
    waveformVisualizer.setLevel(processor.getCurrentReverbLevel());
    waveformVisualizer.setDegrade(degradeKnob.getValue());
    
    // Update infinite indicator visibility
    float decayValue = static_cast<float>(decayKnob.getValue());
    bool isInfinite = decayValue > 29.0f;
    
    // Animate the infinite symbol
    float targetAlpha = isInfinite ? 1.0f : 0.0f;
    float currentAlpha = infiniteIndicator.getAlpha();
    float newAlpha = currentAlpha + (targetAlpha - currentAlpha) * 0.1f;
    infiniteIndicator.setAlpha(newAlpha);
    
    // Pulse the infinite symbol when active
    if (isInfinite)
    {
        static float pulsePhase = 0.0f;
        pulsePhase += 0.15f;
        float pulse = 0.7f + 0.3f * std::sin(pulsePhase);
        infiniteIndicator.setColour(juce::Label::textColourId, 
            dirtLookAndFeel.shimmerColor.withMultipliedBrightness(pulse));
    }
}
