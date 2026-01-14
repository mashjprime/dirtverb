#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <cmath>

/**
 * DirtLookAndFeel - Experimental "Corrupted Crystal" visual theme
 * 
 * Design elements:
 * - Deep purple-to-black gradient backgrounds
 * - Glitchy knob rendering that intensifies with parameter values
 * - CRT-style text glow
 * - Hot pink and cyan accent colors
 */
class DirtLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DirtLookAndFeel()
    {
        // Set up the colour scheme
        setColour(juce::Slider::rotarySliderFillColourId, accent);
        setColour(juce::Slider::rotarySliderOutlineColourId, backgroundDark);
        setColour(juce::Slider::thumbColourId, accent);
        setColour(juce::Label::textColourId, textColor);
        setColour(juce::ResizableWindow::backgroundColourId, backgroundDark);
    }

    // Main colours
    juce::Colour backgroundDark   = juce::Colour(0xFF0d0d14);  // Near black with purple tint
    juce::Colour backgroundMid    = juce::Colour(0xFF1a1a2e);  // Deep purple
    juce::Colour accent           = juce::Colour(0xFFe94560);  // Hot pink
    juce::Colour shimmerColor     = juce::Colour(0xFF00fff5);  // Cyan shimmer
    juce::Colour degradeColor     = juce::Colour(0xFFff6b35);  // Orange grit
    juce::Colour textColor        = juce::Colour(0xFFe0e0e0);  // Light gray text
    juce::Colour knobTrack        = juce::Colour(0xFF2a2a4a);  // Dark purple for track

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        const float radius = static_cast<float>(juce::jmin(width / 2, height / 2)) - 4.0f;
        const float centreX = static_cast<float>(x + width) * 0.5f;
        const float centreY = static_cast<float>(y + height) * 0.5f;
        const float rx = centreX - radius;
        const float ry = centreY - radius;
        const float rw = radius * 2.0f;
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Get parameter name for color selection
        juce::String paramId = slider.getName();
        juce::Colour knobColor = accent;
        
        if (paramId.containsIgnoreCase("shimmer"))
            knobColor = shimmerColor;
        else if (paramId.containsIgnoreCase("degrade") || paramId.containsIgnoreCase("fold") || paramId.containsIgnoreCase("dirt"))
            knobColor = degradeColor;

        // Background circle with subtle gradient
        {
            juce::ColourGradient gradient(
                backgroundMid.brighter(0.1f), centreX, centreY - radius,
                backgroundDark, centreX, centreY + radius, false);
            g.setGradientFill(gradient);
            g.fillEllipse(rx, ry, rw, rw);
        }

        // Outer track ring
        {
            g.setColour(knobTrack);
            juce::Path trackArc;
            trackArc.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                                   0.0f, rotaryStartAngle, rotaryEndAngle, true);
            g.strokePath(trackArc, juce::PathStrokeType(3.0f));
        }

        // Value arc
        {
            g.setColour(knobColor);
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius - 2.0f, radius - 2.0f,
                                   0.0f, rotaryStartAngle, angle, true);
            g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // Glitch effect when degrade/fold is high
        float glitchIntensity = 0.0f;
        if (paramId.containsIgnoreCase("degrade") || paramId.containsIgnoreCase("fold"))
        {
            glitchIntensity = sliderPosProportional;
        }

        // Inner knob
        {
            const float innerRadius = radius * 0.65f;
            juce::ColourGradient gradient(
                knobColor.withAlpha(0.3f), centreX, centreY - innerRadius,
                backgroundDark, centreX, centreY + innerRadius, false);
            g.setGradientFill(gradient);
            
            // Add glitch displacement
            float glitchX = (glitchIntensity > 0.5f) ? (std::sin(angle * 13.7f) * glitchIntensity * 3.0f) : 0.0f;
            g.fillEllipse(centreX - innerRadius + glitchX, centreY - innerRadius,
                         innerRadius * 2.0f, innerRadius * 2.0f);
        }

        // Pointer line
        {
            juce::Path pointer;
            const float pointerLength = radius * 0.5f;
            const float pointerThickness = 3.0f;
            pointer.addRectangle(-pointerThickness * 0.5f, -radius + 6.0f,
                                pointerThickness, pointerLength);
            pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
            
            g.setColour(knobColor);
            g.fillPath(pointer);
        }

        // Glow effect
        {
            const float glowRadius = radius + 8.0f;
            juce::ColourGradient glowGradient(
                knobColor.withAlpha(sliderPosProportional * 0.15f), centreX, centreY,
                knobColor.withAlpha(0.0f), centreX + glowRadius, centreY, true);
            g.setGradientFill(glowGradient);
            g.fillEllipse(centreX - glowRadius, centreY - glowRadius,
                         glowRadius * 2.0f, glowRadius * 2.0f);
        }
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.setColour(textColor);
        
        auto font = getLabelFont(label);
        g.setFont(font);

        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        
        // Draw text with subtle glow
        g.setColour(accent.withAlpha(0.3f));
        g.drawFittedText(label.getText(), textArea.translated(1, 1),
                        label.getJustificationType(), 1);
        
        g.setColour(textColor);
        g.drawFittedText(label.getText(), textArea,
                        label.getJustificationType(), 1);
    }

    juce::Font getLabelFont(juce::Label& label) override
    {
        // Use a monospace-style font for the experimental look
        return juce::Font("Consolas", static_cast<float>(label.getHeight()) * 0.7f, juce::Font::bold);
    }
};
