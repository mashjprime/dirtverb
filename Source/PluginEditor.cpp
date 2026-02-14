#include "PluginProcessor.h"
#include "PluginEditor.h"

// --- CinderContentPanel paint ---

void CinderContentPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(CinderLookAndFeel::colBgPrimary));

    if (laf == nullptr) return;

    const int pad = 16;
    int w = getWidth();

    // Header bar (36px)
    g.setFont(laf->getHeaderFont());
    g.setColour(juce::Colour(CinderLookAndFeel::colTextPrimary));
    g.drawText("CINDER", pad, 8, 200, 22, juce::Justification::centredLeft);

    g.setFont(laf->getBrandFont());
    g.setColour(juce::Colour(CinderLookAndFeel::colTextDim));
    g.drawText("SUBSTRATE AUDIO", w - 140 - pad, 12, 140, 14, juce::Justification::centredRight);

    // Section titles and dividers
    const char* titles[] = { "REVERB", "FIRE", "OUTPUT" };

    for (int i = 0; i < 3; ++i)
    {
        int sy = sectionYPositions[i];
        if (sy <= 0) continue;

        // Divider line above section
        g.setColour(juce::Colour(CinderLookAndFeel::colBorder));
        g.drawHorizontalLine(sy, static_cast<float>(pad), static_cast<float>(w - pad));

        // Section title
        g.setFont(laf->getSectionTitleFont());
        g.setColour(juce::Colour(CinderLookAndFeel::colAccent));
        g.drawText(titles[i], pad, sy + 2, 200, 14, juce::Justification::centredLeft);
    }

    // Footer
    g.setFont(laf->getBrandFont());
    g.setColour(juce::Colour(CinderLookAndFeel::colTextDim));
    g.drawText("v1.0", pad, getHeight() - 20, 60, 14, juce::Justification::centredLeft);
}

// --- Editor implementation ---

CinderEditor::CinderEditor(CinderProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      waveformVisualizer(p.currentReverbLevel,
                         *p.apvts.getRawParameterValue("decay"),
                         *p.apvts.getRawParameterValue("burn")),
      outputMeter(p.outputRmsLevel, p.outputPeakLevel)
{
    setLookAndFeel(&cinderLook);
    contentPanel.laf = &cinderLook;

    addAndMakeVisible(contentPanel);

    // Visualizer
    contentPanel.addAndMakeVisible(waveformVisualizer);

    // Output meter
    contentPanel.addAndMakeVisible(outputMeter);

    // Helpers
    auto addKnob = [this](CinderKnob& slider, juce::Label& label,
                          const juce::String& text, const juce::String& paramId,
                          std::unique_ptr<SliderAtt>& att) {
        contentPanel.addAndMakeVisible(slider);
        slider.setTooltip(text);
        att = std::make_unique<SliderAtt>(processor.apvts, paramId, slider);
        setupLabel(label, text);
    };

    // Reverb section
    addKnob(decayKnob,   decayLabel,   "DECAY",   "decay",   decayAtt);
    addKnob(shimmerKnob, shimmerLabel, "SHIMMER", "shimmer", shimmerAtt);
    addKnob(sizeKnob,    sizeLabel,    "SIZE",    "size",    sizeAtt);

    // Fire section
    addKnob(driveKnob, driveLabel, "DRIVE", "drive", driveAtt);
    addKnob(burnKnob,  burnLabel,  "BURN",  "burn",  burnAtt);

    // Output section
    addKnob(duckKnob, duckLabel, "DUCK", "duck", duckAtt);
    addKnob(mixKnob,  mixLabel,  "MIX",  "mix",  mixAtt);

    // Resizable (aspect-ratio locked)
    constrainer.setFixedAspectRatio(static_cast<double>(designW) / static_cast<double>(designH));
    constrainer.setSizeLimits(
        static_cast<int>(designW * 0.8), static_cast<int>(designH * 0.8),
        static_cast<int>(designW * 1.4), static_cast<int>(designH * 1.4));
    setConstrainer(&constrainer);
    setResizable(true, true);

    setSize(designW, designH);
}

CinderEditor::~CinderEditor()
{
    setLookAndFeel(nullptr);
}

void CinderEditor::setupLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(cinderLook.getLabelFont());
    label.setColour(juce::Label::textColourId, juce::Colour(CinderLookAndFeel::colTextSecondary));
    contentPanel.addAndMakeVisible(label);
}

void CinderEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(CinderLookAndFeel::colBgPrimary));
}

void CinderEditor::resized()
{
    auto bounds = getLocalBounds();

    // Scale content panel to fit window, maintaining design dimensions internally
    float scaleX = static_cast<float>(bounds.getWidth()) / static_cast<float>(designW);
    float scaleY = static_cast<float>(bounds.getHeight()) / static_cast<float>(designH);
    float scale = juce::jmin(scaleX, scaleY);

    contentPanel.setTransform(juce::AffineTransform::scale(scale));
    contentPanel.setBounds(0, 0, designW, designH);

    // --- All layout below at design dimensions (520 x 440) ---
    const int pad = 16;
    const int labelH = 14;
    const int knobS = 55;
    int y = 0;

    // Header: 36px
    y += 36;

    // Waveform Visualizer: 76px
    waveformVisualizer.setBounds(pad, y, designW - pad * 2, 76);
    y += 80;

    // --- REVERB section (DECAY, SHIMMER, SIZE) ---
    contentPanel.sectionYPositions[0] = y;
    y += 18;
    {
        int numKnobs = 3;
        int totalW = designW - pad * 2;
        int spacing = (totalW - numKnobs * knobS) / (numKnobs + 1);
        int kx = pad + spacing;

        auto placeKnob = [&](juce::Label& label, juce::Slider& slider) {
            label.setBounds(kx, y, knobS, labelH);
            slider.setBounds(kx, y + labelH, knobS, knobS);
            kx += knobS + spacing;
        };

        placeKnob(decayLabel, decayKnob);
        placeKnob(shimmerLabel, shimmerKnob);
        placeKnob(sizeLabel, sizeKnob);
    }
    y += knobS + labelH + 4;

    // --- FIRE section (DRIVE, BURN) ---
    contentPanel.sectionYPositions[1] = y;
    y += 18;
    {
        int numKnobs = 2;
        int totalW = designW - pad * 2;
        int spacing = (totalW - numKnobs * knobS) / (numKnobs + 1);
        int kx = pad + spacing;

        auto placeKnob = [&](juce::Label& label, juce::Slider& slider) {
            label.setBounds(kx, y, knobS, labelH);
            slider.setBounds(kx, y + labelH, knobS, knobS);
            kx += knobS + spacing;
        };

        placeKnob(driveLabel, driveKnob);
        placeKnob(burnLabel, burnKnob);
    }
    y += knobS + labelH + 4;

    // --- OUTPUT section (DUCK, MIX + OutputMeter) ---
    contentPanel.sectionYPositions[2] = y;
    y += 18;
    {
        int totalW = designW - pad * 2;
        int meterW = 20;
        int contentW = knobS * 2 + 30 + meterW;  // 2 knobs + gap + meter
        int startX = pad + (totalW - contentW) / 2;

        duckLabel.setBounds(startX, y, knobS, labelH);
        duckKnob.setBounds(startX, y + labelH, knobS, knobS);

        int mixX = startX + knobS + 30;
        mixLabel.setBounds(mixX, y, knobS, labelH);
        mixKnob.setBounds(mixX, y + labelH, knobS, knobS);

        int meterX = mixX + knobS + 20;
        outputMeter.setBounds(meterX, y, meterW, knobS + labelH);
    }
}
