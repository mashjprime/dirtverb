#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <atomic>

/**
 * WaveformVisualizer - Real-time reverb tail visualization
 *
 * Displays the wet signal level with visual corruption effects
 * that match the DEGRADE parameter intensity.
 * Substrate Audio palette: #111111 bg, #1F1F1F border, #C4502A accent
 *
 * Reads level from an atomic<float> reference (set by processor).
 * Degrade amount and infinite state are polled each frame via setters.
 */
class WaveformVisualizer : public juce::Component,
                           public juce::Timer
{
public:
    WaveformVisualizer(std::atomic<float>& levelSource,
                       std::atomic<float>& decayParamSource,
                       std::atomic<float>& degradeParamSource)
        : levelRef(levelSource),
          decayParamRef(decayParamSource),
          degradeParamRef(degradeParamSource)
    {
        startTimerHz(30); // 30fps refresh
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Solid background (Substrate secondary)
        g.setColour(juce::Colour(0xFF111111));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Border (Substrate border)
        g.setColour(juce::Colour(0xFF1F1F1F));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 4.0f, 1.0f);

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

            // Color: Cinder accent interpolated with warm white
            juce::Colour barColor = juce::Colour(0xFFC4502A).interpolatedWith(
                juce::Colour(0xFFE8E4DF), level);

            // Apply glitch effect based on degrade
            float glitchOffset = 0.0f;
            if (degradeAmount > 0.3f)
            {
                glitchOffset = std::sin(static_cast<float>(i) * 0.7f + glitchPhase)
                             * degradeAmount * 5.0f;
            }

            // Skip some bars for digital artifact look when degraded
            if (degradeAmount > 0.6f && (i % 3 == 0))
                continue;

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

        // Infinite indicator (pulsing symbol in top-right)
        if (isInfinite)
        {
            float pulse = 0.7f + 0.3f * std::sin(glitchPhase * 0.5f);
            g.setColour(juce::Colour(0xFFC4502A).withMultipliedBrightness(pulse));
            auto titleArea = bounds.reduced(8.0f);
            g.drawText(juce::CharPointer_UTF8("\xe2\x88\x9e"), titleArea,
                       juce::Justification::topRight);
        }
    }

    void timerCallback() override
    {
        // Read level from processor atomic
        float currentLevel = levelRef.load(std::memory_order_relaxed);

        // Update history
        levelHistory[historyWritePos] = currentLevel;
        historyWritePos = (historyWritePos + 1) % historySize;

        // Read degrade amount for glitch effects
        degradeAmount = degradeParamRef.load(std::memory_order_relaxed);

        // Check infinite mode (decay > 29.0s)
        isInfinite = decayParamRef.load(std::memory_order_relaxed) > 29.0f;

        // Update glitch animation
        glitchPhase += 0.3f;
        if (glitchPhase > juce::MathConstants<float>::twoPi * 10.0f)
            glitchPhase -= juce::MathConstants<float>::twoPi * 10.0f;

        repaint();
    }

private:
    std::atomic<float>& levelRef;
    std::atomic<float>& decayParamRef;
    std::atomic<float>& degradeParamRef;

    float degradeAmount = 0.0f;
    bool isInfinite = false;
    float glitchPhase = 0.0f;

    static constexpr int historySize = 64;
    std::array<float, historySize> levelHistory{};
    int historyWritePos = 0;
};
