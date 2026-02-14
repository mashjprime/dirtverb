#pragma once

#include <cmath>
#include <algorithm>

/**
 * LofiDegrader - Sample rate reduction and bit crushing
 * 
 * Creates lo-fi character by:
 * - Reducing effective sample rate (sample & hold)
 * - Reducing bit depth (quantization)
 * 
 * The DEGRADE parameter controls both simultaneously:
 * - 0% = 44.1kHz, 16-bit (clean)
 * - 100% = 4kHz, 4-bit (extremely crushed)
 */
class LofiDegrader
{
public:
    LofiDegrader() = default;

    void prepare(double sampleRate)
    {
        this->actualSampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        phase = 0.0f;
        holdSampleL = 0.0f;
    }

    void setDegrade(float amount)
    {
        // amount: 0.0 to 1.0
        degradeAmount = std::clamp(amount, 0.0f, 1.0f);

        // Map to target sample rate: 44100 Hz -> 4000 Hz
        // Using exponential curve for more musical control
        float srRatio = std::pow(0.1f, degradeAmount); // 1.0 -> 0.1
        targetSampleRate = static_cast<float>(actualSampleRate) * srRatio;
        targetSampleRate = std::max(targetSampleRate, 4000.0f);

        // Map to bit depth: 16 bits -> 4 bits
        // Linear interpolation
        targetBitDepth = 16.0f - degradeAmount * 12.0f;
        targetBitDepth = std::max(targetBitDepth, 4.0f);
    }

    float process(float input)
    {
        if (degradeAmount < 0.001f)
        {
            // Bypass when clean
            return input;
        }

        // Sample rate reduction via sample-and-hold
        float phaseIncrement = targetSampleRate / static_cast<float>(actualSampleRate);
        phase += phaseIncrement;

        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            // Sample and bit-crush
            holdSampleL = bitCrush(input, targetBitDepth);
        }

        return holdSampleL;
    }

private:
    double actualSampleRate = 44100.0;
    float targetSampleRate = 44100.0f;
    float targetBitDepth = 16.0f;
    float degradeAmount = 0.0f;

    float phase = 0.0f;
    float holdSampleL = 0.0f;

    float bitCrush(float sample, float bits)
    {
        // Quantize to specified bit depth
        // bits can be fractional for smooth transitions
        float scale = std::pow(2.0f, bits - 1.0f);
        float quantized = std::round(sample * scale) / scale;
        
        // Add subtle dithering to reduce quantization artifacts
        // Only when not at extreme settings
        if (bits > 6.0f)
        {
            // Triangular dither at ~-120dB
            float dither = (randomFloat() + randomFloat() - 1.0f) * (1.0f / scale) * 0.5f;
            quantized += dither;
        }
        
        return quantized;
    }

    // Simple pseudo-random for dithering
    uint32_t randState = 12345;
    float randomFloat()
    {
        randState = randState * 1103515245 + 12345;
        return static_cast<float>(randState & 0x7FFFFFFF) / static_cast<float>(0x7FFFFFFF);
    }
};
