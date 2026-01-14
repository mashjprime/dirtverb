#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

/**
 * WaveformVisualizer - Real-time reverb tail visualization
 * 
 * Displays the wet signal level with visual corruption effects
 * that match the DEGRADE parameter intensity.
 */
class WaveformVisualizer : public juce::Component,
                           public juce::Timer
{
public:
    WaveformVisualizer()
    {
        startTimerHz(30); // 30fps refresh
    }

    void setLevel(float level)
    {
        currentLevel = level;
    }

    void setDegrade(float amount)
    {
        degradeAmount = amount;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        juce::ColourGradient bgGradient(
            juce::Colour(0xFF0d0d14), bounds.getX(), bounds.getY(),
            juce::Colour(0xFF1a1a2e), bounds.getX(), bounds.getBottom(),
            false);
        g.setGradientFill(bgGradient);
        g.fillRoundedRectangle(bounds, 8.0f);

        // Border
        g.setColour(juce::Colour(0xFF2a2a4a));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 2.0f);

        // Draw waveform history
        const float barWidth = bounds.getWidth() / static_cast<float>(historySize);
        const float maxBarHeight = bounds.getHeight() * 0.8f;

        for (int i = 0; i < historySize; ++i)
        {
            int histIdx = (historyWritePos + i) % historySize;
            float level = levelHistory[histIdx];
            float barHeight = level * maxBarHeight;

            float x = bounds.getX() + static_cast<float>(i) * barWidth;
            float y = bounds.getCentreY() - barHeight * 0.5f;

            // Color based on level and degrade
            juce::Colour barColor;
            if (degradeAmount > 0.5f)
            {
                // Orange-red for degraded
                barColor = juce::Colour(0xFFff6b35).interpolatedWith(
                    juce::Colour(0xFFe94560), level);
            }
            else
            {
                // Cyan for clean
                barColor = juce::Colour(0xFF00fff5).interpolatedWith(
                    juce::Colour(0xFFe94560), level);
            }

            // Apply glitch effect based on degrade
            float glitchOffset = 0.0f;
            if (degradeAmount > 0.3f)
            {
                // Random vertical offset
                glitchOffset = std::sin(static_cast<float>(i) * 0.7f + glitchPhase) 
                             * degradeAmount * 5.0f;
            }

            // Draw bar with potential scanline effect
            if (degradeAmount > 0.6f && (i % 3 == 0))
            {
                // Skip some bars for digital artifact look
                continue;
            }

            g.setColour(barColor.withAlpha(0.8f));
            g.fillRect(x + 1.0f, y + glitchOffset, 
                      barWidth - 2.0f, barHeight);

            // Glow effect for high levels
            if (level > 0.5f)
            {
                g.setColour(barColor.withAlpha(level * 0.3f));
                g.fillRect(x, y + glitchOffset - 2.0f, 
                          barWidth, barHeight + 4.0f);
            }
        }

        // Scanline overlay when degraded
        if (degradeAmount > 0.4f)
        {
            g.setColour(juce::Colours::black.withAlpha(degradeAmount * 0.1f));
            for (float scanY = bounds.getY(); scanY < bounds.getBottom(); scanY += 3.0f)
            {
                g.fillRect(bounds.getX(), scanY, bounds.getWidth(), 1.0f);
            }
        }

        // Title
        g.setColour(juce::Colour(0xFFe0e0e0));
        g.setFont(juce::Font("Consolas", 11.0f, juce::Font::bold));
        
        // Infinite symbol when at max level
        juce::String title = (currentLevel > 0.95f) ? 
            juce::CharPointer_UTF8("\xe2\x88\x9e REVERB") : "REVERB";
        g.drawText(title, bounds.reduced(8.0f), juce::Justification::topLeft);
    }

    void timerCallback() override
    {
        // Update history
        levelHistory[historyWritePos] = currentLevel;
        historyWritePos = (historyWritePos + 1) % historySize;
        
        // Update glitch animation
        glitchPhase += 0.3f;
        if (glitchPhase > juce::MathConstants<float>::twoPi)
            glitchPhase -= juce::MathConstants<float>::twoPi;

        repaint();
    }

private:
    float currentLevel = 0.0f;
    float degradeAmount = 0.0f;
    float glitchPhase = 0.0f;

    static constexpr int historySize = 64;
    std::array<float, historySize> levelHistory{};
    int historyWritePos = 0;
};
