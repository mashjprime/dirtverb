#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
 * ShimmerReverb - 8-channel Feedback Delay Network with pitch-shifted feedback
 * 
 * Architecture:
 * - Input diffusion (4 allpass filters to smear transients)
 * - 8 parallel delay lines with prime-ish lengths
 * - Hadamard matrix mixing for energy-preserving feedback
 * - Pitch shifter in feedback loop for shimmer effect
 * - Damping filters for natural high-frequency decay
 */
class ShimmerReverb
{
public:
    ShimmerReverb() = default;

    void prepare(double sampleRate, int maxBlockSize)
    {
        this->sampleRate = sampleRate;

        // Calculate delay times based on sample rate
        // Using prime-ish numbers for inharmonic density
        const std::array<float, 8> baseDelayMs = {35.3f, 36.7f, 33.8f, 32.3f, 29.0f, 30.8f, 27.0f, 25.3f};
        
        for (int i = 0; i < 8; ++i)
        {
            int delaySamples = static_cast<int>(baseDelayMs[i] * sampleRate / 1000.0f);
            delayLines[i].setMaximumDelayInSamples(delaySamples * 4); // Extra headroom for size modulation
            delayLines[i].prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
            baseDelayTimes[i] = delaySamples;
        }

        // Input diffusers (allpass chain)
        for (int i = 0; i < 4; ++i)
        {
            inputDiffusers[i].prepare({sampleRate, static_cast<juce::uint32>(maxBlockSize), 1});
            inputDiffusers[i].setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.05)); // 50ms max
        }

        // Damping filters (one-pole lowpass per delay line)
        for (auto& filter : dampingFilters)
        {
            filter = 0.0f;
        }

        // Pitch shifter for shimmer (granular-style)
        pitchShiftBuffer.resize(static_cast<size_t>(sampleRate * 0.5)); // 500ms buffer
        std::fill(pitchShiftBuffer.begin(), pitchShiftBuffer.end(), 0.0f);
        pitchShiftWritePos = 0;
        pitchShiftReadPos = 0.0f;

        reset();
    }

    void reset()
    {
        for (auto& dl : delayLines)
            dl.reset();
        for (auto& diff : inputDiffusers)
            diff.reset();
        for (auto& state : delayLineStates)
            state = 0.0f;
        std::fill(pitchShiftBuffer.begin(), pitchShiftBuffer.end(), 0.0f);
    }

    void setParameters(float decaySeconds, float shimmerAmount, float roomSize)
    {
        // Convert decay time to feedback gain
        // Using RT60 formula: gain = 10^(-3 * delayTime / RT60)
        // Cap feedback well below unity to prevent runaway
        if (decaySeconds > 50.0f)
        {
            // "Infinite" mode - still slightly below unity for stability
            feedbackGain = 0.9985f;
        }
        else
        {
            // Average delay time ~30ms
            const float avgDelaySeconds = 0.030f;
            feedbackGain = std::pow(10.0f, -3.0f * avgDelaySeconds / decaySeconds);
            // Cap at 0.985 to prevent runaway even at high decay
            feedbackGain = std::clamp(feedbackGain, 0.0f, 0.985f);
        }

        this->shimmerMix = shimmerAmount;
        this->roomSize = std::clamp(roomSize, 0.0f, 1.0f);

        // Damping: higher roomSize = less damping (brighter)
        // Also reduce damping coefficient to absorb more energy
        dampingCoeff = 0.2f + roomSize * 0.4f;
        
        // Compensate feedback for shimmer energy injection
        // Shimmer adds energy, so reduce feedback proportionally
        shimmerCompensation = 1.0f - (shimmerAmount * 0.15f);
    }

    float process(float input)
    {
        // 1. Input diffusion (smears transients for smoother reverb)
        float diffused = input;
        const std::array<float, 4> diffuserDelays = {0.0042f, 0.0036f, 0.0029f, 0.0023f}; // seconds
        const float diffuserGain = 0.6f;

        for (int i = 0; i < 4; ++i)
        {
            float delaySamples = static_cast<float>(diffuserDelays[i] * sampleRate);
            float delayed = inputDiffusers[i].popSample(0, delaySamples);
            float toWrite = diffused + delayed * diffuserGain;
            inputDiffusers[i].pushSample(0, toWrite);
            diffused = delayed - diffused * diffuserGain;
        }

        // 2. Read from delay lines and apply Hadamard mixing
        std::array<float, 8> delayOutputs;
        for (int i = 0; i < 8; ++i)
        {
            // Modulate delay time by room size
            float delayTime = baseDelayTimes[i] * (0.5f + roomSize);
            delayOutputs[i] = delayLines[i].popSample(0, delayTime);
        }

        // 3. Hadamard matrix mixing (8x8, normalized)
        // This creates dense, energy-preserving feedback
        std::array<float, 8> mixed = hadamardMix(delayOutputs);

        // 4. Apply damping (one-pole lowpass), soft limiting, and feedback gain
        for (int i = 0; i < 8; ++i)
        {
            // Simple one-pole lowpass for damping
            dampingFilters[i] = dampingFilters[i] + dampingCoeff * (mixed[i] - dampingFilters[i]);
            
            // Soft limiter in feedback loop to prevent runaway
            float limited = softLimit(dampingFilters[i]);
            
            // Apply feedback gain with shimmer compensation
            mixed[i] = limited * feedbackGain * shimmerCompensation;
        }

        // 5. Apply shimmer (pitch shift) in feedback
        float pitchShifted = processPitchShift(mixed[0]); // Use first channel for shimmer
        
        // 6. Write to delay lines (input + feedback, with shimmer blended)
        float inputContribution = diffused / 8.0f;
        for (int i = 0; i < 8; ++i)
        {
            // Reduced shimmer contribution to prevent energy buildup
            float shimmerContrib = (i < 4) ? pitchShifted * shimmerMix * 0.25f : 0.0f;
            float toWrite = mixed[i] + inputContribution + shimmerContrib;
            // Final safety limiter before writing to delay
            delayLines[i].pushSample(0, softLimit(toWrite));
        }

        // 7. Output: sum all delay lines
        float output = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            output += delayOutputs[i];
        }
        return output * 0.25f; // Normalize output level
    }

private:
    double sampleRate = 44100.0;
    
    // 8-channel FDN
    std::array<juce::dsp::DelayLine<float>, 8> delayLines;
    std::array<int, 8> baseDelayTimes;
    std::array<float, 8> delayLineStates{};
    
    // Input diffusers
    std::array<juce::dsp::DelayLine<float>, 4> inputDiffusers;
    
    // Damping filters (simple one-pole state)
    std::array<float, 8> dampingFilters{};
    float dampingCoeff = 0.7f;
    
    // Parameters
    float feedbackGain = 0.85f;
    float shimmerMix = 0.0f;
    float roomSize = 0.5f;
    float shimmerCompensation = 1.0f;

    // Pitch shifter state (simple granular)
    std::vector<float> pitchShiftBuffer;
    int pitchShiftWritePos = 0;
    float pitchShiftReadPos = 0.0f;

    // Soft limiter to prevent runaway - uses tanh for smooth limiting
    float softLimit(float x)
    {
        // Threshold above which we start limiting
        const float threshold = 0.8f;
        
        if (std::abs(x) < threshold)
            return x;
        
        // Soft saturation above threshold using tanh
        float sign = (x > 0.0f) ? 1.0f : -1.0f;
        float excess = std::abs(x) - threshold;
        float limited = threshold + (1.0f - threshold) * std::tanh(excess * 2.0f);
        return sign * limited;
    }

    // Hadamard matrix multiplication (8x8)
    std::array<float, 8> hadamardMix(const std::array<float, 8>& input)
    {
        // Normalized 8x8 Hadamard matrix
        // Each row/column has entries of +1 or -1, normalized by 1/sqrt(8)
        const float norm = 1.0f / std::sqrt(8.0f);
        
        std::array<float, 8> output;
        
        // Using the recursive Hadamard structure
        // H8 = [[H4, H4], [H4, -H4]]
        for (int i = 0; i < 8; ++i)
        {
            float sum = 0.0f;
            for (int j = 0; j < 8; ++j)
            {
                // Hadamard entry: (-1)^(popcount(i & j))
                int bits = i & j;
                int popcount = 0;
                while (bits) { popcount += bits & 1; bits >>= 1; }
                float sign = (popcount % 2 == 0) ? 1.0f : -1.0f;
                sum += sign * input[j];
            }
            output[i] = sum * norm;
        }
        
        return output;
    }

    // Simple granular pitch shifter (+1 octave)
    float processPitchShift(float input)
    {
        // Write to buffer
        pitchShiftBuffer[pitchShiftWritePos] = input;
        pitchShiftWritePos = (pitchShiftWritePos + 1) % static_cast<int>(pitchShiftBuffer.size());

        // Read at double speed for octave up
        const float pitchRatio = 2.0f;
        pitchShiftReadPos += pitchRatio;
        if (pitchShiftReadPos >= static_cast<float>(pitchShiftBuffer.size()))
            pitchShiftReadPos -= static_cast<float>(pitchShiftBuffer.size());

        // Linear interpolation for smooth reading
        int readIdx = static_cast<int>(pitchShiftReadPos);
        float frac = pitchShiftReadPos - readIdx;
        int nextIdx = (readIdx + 1) % static_cast<int>(pitchShiftBuffer.size());
        
        float sample = pitchShiftBuffer[readIdx] * (1.0f - frac) + 
                       pitchShiftBuffer[nextIdx] * frac;

        // Apply window function to reduce artifacts (simple cosine crossfade)
        const int grainSize = 512;
        int grainPos = static_cast<int>(pitchShiftReadPos) % grainSize;
        float window = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * grainPos / grainSize);
        
        return sample * window;
    }
};
