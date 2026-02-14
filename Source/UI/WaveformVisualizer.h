#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <atomic>

/**
 * WaveformVisualizer - Real-time reverb tail visualization
 *
 * Displays the wet signal level with heat distortion effects
 * that match the BURN parameter intensity.
 * Substrate Audio palette: #111111 bg, #1F1F1F border, #C4502A accent
 *
 * Reads level from an atomic<float> reference (set by processor).
 * Burn amount and infinite state are polled each frame via setters.
 */
class WaveformVisualizer : public juce::Component,
                           public juce::Timer
{
public:
    WaveformVisualizer(std::atomic<float>& levelSource,
                       std::atomic<float>& decayParamSource,
                       std::atomic<float>& burnParamSource)
        : levelRef(levelSource),
          decayParamRef(decayParamSource),
          burnParamRef(burnParamSource)
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

            // Color: interpolate from accent (cool) toward bright ember (hot) with burn
            juce::Colour baseColor = juce::Colour(0xFFC4502A);
            juce::Colour hotColor = juce::Colour(0xFFE8A040);  // bright amber/orange
            juce::Colour barColor = baseColor.interpolatedWith(hotColor, burnAmount * level);

            // Heat shimmer: sinusoidal vertical displacement when burning
            float heatOffset = 0.0f;
            if (burnAmount > 0.1f)
            {
                heatOffset = std::sin(static_cast<float>(i) * 0.5f + heatPhase)
                           * burnAmount * 3.0f;
            }

            g.setColour(barColor.withAlpha(0.8f));
            g.fillRect(x + 1.0f, y + heatOffset,
                      barWidth - 2.0f, barHeight);

            // Glow effect — intensifies with burn
            if (level > 0.3f)
            {
                float glowAlpha = level * (0.2f + burnAmount * 0.3f);
                g.setColour(barColor.withAlpha(glowAlpha));
                g.fillRect(x, y + heatOffset - 2.0f,
                          barWidth, barHeight + 4.0f);
            }
        }

        // Infinite indicator (pulsing symbol in top-right)
        if (isInfinite)
        {
            float pulse = 0.7f + 0.3f * std::sin(heatPhase * 0.5f);
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

        // Read burn amount for heat effects
        burnAmount = burnParamRef.load(std::memory_order_relaxed);

        // Check infinite mode (decay > 29.0s)
        isInfinite = decayParamRef.load(std::memory_order_relaxed) > 29.0f;

        // Update heat animation phase
        heatPhase += 0.3f;
        if (heatPhase > juce::MathConstants<float>::twoPi * 10.0f)
            heatPhase -= juce::MathConstants<float>::twoPi * 10.0f;

        repaint();
    }

private:
    std::atomic<float>& levelRef;
    std::atomic<float>& decayParamRef;
    std::atomic<float>& burnParamRef;

    float burnAmount = 0.0f;
    bool isInfinite = false;
    float heatPhase = 0.0f;

    static constexpr int historySize = 64;
    std::array<float, historySize> levelHistory{};
    int historyWritePos = 0;
};
