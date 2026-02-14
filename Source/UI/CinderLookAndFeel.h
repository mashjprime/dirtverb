#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

// Substrate Audio visual identity — Cinder variant
// Ember red-orange accent (#C4502A), DM Sans + Space Mono, #0A0A0A background
class CinderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Substrate color palette
    static constexpr juce::uint32 colBgPrimary    = 0xFF0A0A0A;
    static constexpr juce::uint32 colBgSecondary  = 0xFF111111;
    static constexpr juce::uint32 colBgTertiary   = 0xFF1A1A1A;
    static constexpr juce::uint32 colTextPrimary   = 0xFFE8E4DF;
    static constexpr juce::uint32 colTextSecondary = 0xFF8A8580;
    static constexpr juce::uint32 colTextDim       = 0xFF4A4745;
    static constexpr juce::uint32 colBorder        = 0xFF1F1F1F;
    static constexpr juce::uint32 colBorderLight   = 0xFF2A2A2A;
    static constexpr juce::uint32 colAccent        = 0xFFC4502A;
    static constexpr juce::uint32 colTrack         = 0xFF2A2A2A;
    static constexpr juce::uint32 colKnobBody      = 0xFF111111;

    CinderLookAndFeel()
    {
        // Load embedded fonts
        dmSansLight = juce::Typeface::createSystemTypefaceFor(
            BinaryData::DMSansLight_ttf, BinaryData::DMSansLight_ttfSize);
        dmSansRegular = juce::Typeface::createSystemTypefaceFor(
            BinaryData::DMSansRegular_ttf, BinaryData::DMSansRegular_ttfSize);
        dmSansMedium = juce::Typeface::createSystemTypefaceFor(
            BinaryData::DMSansMedium_ttf, BinaryData::DMSansMedium_ttfSize);
        spaceMono = juce::Typeface::createSystemTypefaceFor(
            BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
        spaceMonoBold = juce::Typeface::createSystemTypefaceFor(
            BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);

        // Set default colors
        setColour(juce::Label::textColourId, juce::Colour(colTextSecondary));
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(colBgTertiary));
        setColour(juce::TextEditor::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::TextEditor::outlineColourId, juce::Colour(colBorder));
        setColour(juce::ToggleButton::textColourId, juce::Colour(colTextSecondary));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(colAccent));
        setColour(juce::TooltipWindow::backgroundColourId, juce::Colour(colBgTertiary));
        setColour(juce::TooltipWindow::textColourId, juce::Colour(colTextPrimary));
        setColour(juce::TooltipWindow::outlineColourId, juce::Colour(colBorder));
    }

    // --- Font accessors (JUCE 8 FontOptions API) ---
    juce::Font getLabelFont() const
    {
        return juce::Font(juce::FontOptions(dmSansRegular).withHeight(11.0f));
    }
    juce::Font getSectionTitleFont() const
    {
        return juce::Font(juce::FontOptions(spaceMono).withHeight(10.0f))
            .withExtraKerningFactor(0.15f);
    }
    juce::Font getValueFont() const
    {
        return juce::Font(juce::FontOptions(spaceMono).withHeight(10.0f));
    }
    juce::Font getHeaderFont() const
    {
        return juce::Font(juce::FontOptions(dmSansLight).withHeight(16.0f));
    }
    juce::Font getBrandFont() const
    {
        return juce::Font(juce::FontOptions(spaceMono).withHeight(9.0f))
            .withExtraKerningFactor(0.1f);
    }

    // --- Rotary knob ---
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();
        auto arcRadius = radius - 4.0f;

        // Arc track (dim)
        juce::Path trackPath;
        trackPath.addCentredArc(centre.getX(), centre.getY(), arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(colTrack));
        g.strokePath(trackPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

        // Value arc (accent fill)
        if (slider.isEnabled())
        {
            juce::Path valPath;
            valPath.addCentredArc(centre.getX(), centre.getY(), arcRadius, arcRadius,
                                  0.0f, rotaryStartAngle, toAngle, true);

            // Hover glow
            if (slider.isMouseOverOrDragging())
            {
                g.setColour(juce::Colour(colAccent).withAlpha(0.08f));
                juce::Path glowCircle;
                glowCircle.addEllipse(centre.getX() - radius, centre.getY() - radius,
                                      radius * 2.0f, radius * 2.0f);
                g.fillPath(glowCircle);
            }

            g.setColour(juce::Colour(colAccent));
            g.strokePath(valPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        // Knob body
        auto knobRadius = radius * 0.55f;
        g.setColour(juce::Colour(colKnobBody));
        g.fillEllipse(centre.getX() - knobRadius, centre.getY() - knobRadius,
                      knobRadius * 2.0f, knobRadius * 2.0f);
        g.setColour(juce::Colour(colBorderLight));
        g.drawEllipse(centre.getX() - knobRadius, centre.getY() - knobRadius,
                      knobRadius * 2.0f, knobRadius * 2.0f, 1.0f);

        // Warm white indicator
        juce::Path indicator;
        indicator.addRectangle(-1.5f, -knobRadius + 3.0f, 3.0f, 5.0f);
        indicator.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre));
        g.setColour(juce::Colour(colTextPrimary));
        g.fillPath(indicator);
    }

    // --- Linear slider (fader) ---
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                          const juce::Slider::SliderStyle style, juce::Slider& /*slider*/) override
    {
        bool isVertical = (style == juce::Slider::LinearVertical);

        juce::Rectangle<float> track;
        if (isVertical)
            track = juce::Rectangle<float>((float)x + width * 0.5f - 2.0f, (float)y, 4.0f, (float)height);
        else
            track = juce::Rectangle<float>((float)x, (float)y + height * 0.5f - 2.0f, (float)width, 4.0f);

        // Track background
        g.setColour(juce::Colour(colTrack));
        g.fillRoundedRectangle(track, 2.0f);

        // Active fill (accent)
        juce::Rectangle<float> fill;
        if (isVertical)
            fill = juce::Rectangle<float>(track.getX(), sliderPos, track.getWidth(),
                                          track.getBottom() - sliderPos);
        else
            fill = juce::Rectangle<float>(track.getX(), track.getY(),
                                          sliderPos - track.getX(), track.getHeight());

        g.setColour(juce::Colour(colAccent));
        g.fillRoundedRectangle(fill, 2.0f);

        // Thumb
        auto thumbSize = 14.0f;
        juce::Point<float> center = isVertical
            ? juce::Point<float>(track.getCentreX(), sliderPos)
            : juce::Point<float>(sliderPos, track.getCentreY());

        // Dark body
        g.setColour(juce::Colour(colKnobBody));
        g.fillEllipse(center.x - thumbSize * 0.5f, center.y - thumbSize * 0.5f,
                      thumbSize, thumbSize);
        // Warm white rim
        g.setColour(juce::Colour(colTextPrimary));
        g.drawEllipse(center.x - thumbSize * 0.5f, center.y - thumbSize * 0.5f,
                      thumbSize, thumbSize, 1.5f);
    }

    // --- Toggle button ---
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto toggleSize = 14.0f;

        // Toggle box
        auto toggleBounds = juce::Rectangle<float>(
            bounds.getX() + 2.0f,
            bounds.getCentreY() - toggleSize * 0.5f,
            toggleSize, toggleSize);

        g.setColour(juce::Colour(colBgTertiary));
        g.fillRoundedRectangle(toggleBounds, 2.0f);
        g.setColour(juce::Colour(colBorderLight));
        g.drawRoundedRectangle(toggleBounds, 2.0f, 1.0f);

        if (button.getToggleState())
        {
            g.setColour(juce::Colour(colAccent));
            g.fillRoundedRectangle(toggleBounds.reduced(3.0f), 1.0f);
        }

        // Label text
        auto textBounds = bounds.withLeft(toggleBounds.getRight() + 4.0f);
        g.setColour(button.getToggleState()
            ? juce::Colour(colTextPrimary) : juce::Colour(colTextSecondary));
        g.setFont(juce::Font(juce::FontOptions(spaceMono).withHeight(10.0f)));
        g.drawText(button.getButtonText(), textBounds, juce::Justification::centredLeft);

        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
        {
            g.setColour(juce::Colour(colAccent).withAlpha(0.08f));
            g.fillRoundedRectangle(bounds, 2.0f);
        }
    }

    // --- Tooltip styling ---
    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText,
                                           juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override
    {
        auto font = juce::Font(juce::FontOptions(spaceMono).withHeight(10.0f));
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, tipText, 0.0f, 0.0f);
        int w = static_cast<int>(std::ceil(
            glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), true).getWidth())) + 16;
        int h = 22;
        return juce::Rectangle<int>(screenPos.x - w / 2, screenPos.y - h - 8, w, h)
            .constrainedWithin(parentArea);
    }

    void drawTooltip(juce::Graphics& g, const juce::String& text,
                     int width, int height) override
    {
        g.setColour(juce::Colour(colBgTertiary));
        g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 3.0f);
        g.setColour(juce::Colour(colBorder));
        g.drawRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, 3.0f, 1.0f);
        g.setColour(juce::Colour(colTextPrimary));
        g.setFont(juce::Font(juce::FontOptions(spaceMono).withHeight(10.0f)));
        g.drawText(text, 0, 0, width, height, juce::Justification::centred);
    }

private:
    juce::Typeface::Ptr dmSansLight, dmSansRegular, dmSansMedium;
    juce::Typeface::Ptr spaceMono, spaceMonoBold;
};
