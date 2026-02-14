#pragma once

#include <cmath>
#include <algorithm>

/**
 * Wavefolder - Triangle wave folding for harmonic generation
 * 
 * Creates rich harmonic content by "folding" the waveform when it
 * exceeds ±1.0, similar to a modular synthesizer wavefolder.
 * 
 * Includes:
 * - Adjustable fold amount (1-10x gain before folding)
 * - DC blocking filter to remove low-frequency buildup
 * - Soft saturation at extreme settings
 */
class Wavefolder
{
public:
    Wavefolder() = default;

    void prepare(double sr)
    {
        sampleRate = sr;

        // DC blocker coefficients (high-pass at ~20Hz)
        const float fc = 20.0f;
        const float wc = 2.0f * 3.14159265f * fc / static_cast<float>(sampleRate);
        dcBlockerCoeff = 1.0f / (1.0f + wc);
        
        reset();
    }

    void reset()
    {
        dcBlockerState = 0.0f;
        prevInput = 0.0f;
    }

    void setFold(float amount)
    {
        // amount: 0.0 to 1.0
        // Map to fold intensity: 1.0 (bypass) to 8.0 (extreme)
        foldAmount = std::clamp(amount, 0.0f, 1.0f);
        
        // Exponential mapping for more musical control
        // 0% = 1.0x (no folding), 100% = 8.0x (extreme harmonics)
        foldGain = 1.0f + foldAmount * 7.0f;
    }

    float process(float input)
    {
        if (foldAmount < 0.001f)
        {
            // Bypass when fold is off
            return input;
        }

        // Apply gain before folding
        float x = input * foldGain;

        // Triangle wave folding algorithm
        // Maps the input to a triangle wave, creating odd harmonics
        x = fold(x);

        // Apply soft saturation to tame extreme peaks
        x = softClip(x);

        // DC blocking (the folding can create DC offset)
        float dcBlocked = dcBlock(x);

        // Compensate for output level
        return dcBlocked * 0.7f;
    }

private:
    double sampleRate = 44100.0;
    float foldAmount = 0.0f;
    float foldGain = 1.0f;

    // DC blocker state
    float dcBlockerCoeff = 0.995f;
    float dcBlockerState = 0.0f;
    float prevInput = 0.0f;

    // Core folding function
    float fold(float x)
    {
        // Reduce to range [-2, 2] first using modulo
        // Then fold within that range
        
        // Normalize to [-2, 2] using floor division
        if (x > 2.0f || x < -2.0f)
        {
            x = std::fmod(x + 2.0f, 4.0f);
            if (x < 0) x += 4.0f;
            x -= 2.0f;
        }

        // Triangle fold: |2 - |x + 2|| - 1
        // This creates the characteristic wavefolder sound
        x = std::abs(std::abs(x) - 2.0f) - 1.0f;

        return x;
    }

    // Soft clipper to prevent harsh clipping at extremes
    float softClip(float x)
    {
        // Cubic soft clipper
        if (x > 1.0f)
            return 1.0f - (1.0f / (x * x + 1.0f));
        else if (x < -1.0f)
            return -1.0f + (1.0f / (x * x + 1.0f));
        else
            return x - (x * x * x) / 3.0f;
    }

    // DC blocking filter
    float dcBlock(float input)
    {
        // High-pass filter to remove DC offset
        float output = input - prevInput + dcBlockerCoeff * dcBlockerState;
        prevInput = input;
        dcBlockerState = output;
        return output;
    }
};
