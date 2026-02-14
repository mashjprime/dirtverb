#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

// Vertical RMS/peak output meter
// Polls atomic levels from processor at 30fps
class OutputMeter : public juce::Component, public juce::Timer
{
public:
    OutputMeter(std::atomic<float>& rmsSource, std::atomic<float>& peakSource)
        : rmsLevel(rmsSource), peakLevel(peakSource)
    {
        startTimerHz(30);
    }

    ~OutputMeter() override { stopTimer(); }

    void timerCallback() override
    {
        float newRms = rmsLevel.load(std::memory_order_relaxed);
        float newPeak = peakLevel.load(std::memory_order_relaxed);

        // Smooth falloff for display
        constexpr float falloff = 0.85f;
        displayRms = (newRms > displayRms) ? newRms : displayRms * falloff;
        displayPeak = (newPeak > displayPeak) ? newPeak : displayPeak * falloff;

        // Peak hold with decay (2s at 30fps = 60 frames)
        if (newPeak >= peakHold)
        {
            peakHold = newPeak;
            peakHoldFrames = 60;
        }
        else if (peakHoldFrames > 0)
        {
            peakHoldFrames--;
        }
        else
        {
            peakHold *= 0.95f;
        }

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Background
        g.setColour(juce::Colour(0xFF111111));
        g.fillRoundedRectangle(bounds, 2.0f);

        // Border
        g.setColour(juce::Colour(0xFF1F1F1F));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);

        float meterH = bounds.getHeight() - 4.0f;
        float meterX = bounds.getX() + 2.0f;
        float meterW = bounds.getWidth() - 4.0f;
        float meterBottom = bounds.getBottom() - 2.0f;

        // Convert to dB for display (clamp to -60..+6)
        float rmsDb = juce::Decibels::gainToDecibels(displayRms, -60.0f);
        float peakHoldDb = juce::Decibels::gainToDecibels(peakHold, -60.0f);

        // Map dB to 0..1 range (-60dB = 0, 0dB = ~0.9, +6dB = 1.0)
        auto dbToNorm = [](float db) {
            return juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 66.0f);
        };

        float rmsNorm = dbToNorm(rmsDb);
        float peakHoldNorm = dbToNorm(peakHoldDb);

        // RMS bar
        float barHeight = rmsNorm * meterH;
        float barY = meterBottom - barHeight;

        // Color: accent for normal, gold for >-6dB, red for clipping
        juce::Colour barColour;
        if (rmsDb > 0.0f)
            barColour = juce::Colour(0xFFD43030); // Clipping red
        else if (rmsDb > -6.0f)
            barColour = juce::Colour(0xFFD4A22A); // Warning gold
        else
            barColour = juce::Colour(0xFFC4502A); // Normal accent (Cinder ember)

        g.setColour(barColour);
        g.fillRect(meterX, barY, meterW, barHeight);

        // Peak hold line
        if (peakHold > 0.001f)
        {
            float holdY = meterBottom - peakHoldNorm * meterH;
            juce::Colour holdColour;
            if (peakHoldDb > 0.0f)
                holdColour = juce::Colour(0xFFD43030);
            else if (peakHoldDb > -6.0f)
                holdColour = juce::Colour(0xFFD4A22A);
            else
                holdColour = juce::Colour(0xFFE8E4DF);

            g.setColour(holdColour);
            g.fillRect(meterX, holdY, meterW, 2.0f);
        }
    }

private:
    std::atomic<float>& rmsLevel;
    std::atomic<float>& peakLevel;

    float displayRms = 0.0f;
    float displayPeak = 0.0f;
    float peakHold = 0.0f;
    int peakHoldFrames = 0;
};
