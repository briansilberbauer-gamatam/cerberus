#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

namespace cerberus
{

/**
    An envelope-controlled state-variable filter.

    The detector is linked across channels and drives an exponential sweep between
    "start" and "start * 2^range". The filter is a topology-preserving transform
    SVF, so it remains stable while the cutoff is being modulated sample by sample.
*/
class AutoWah
{
public:
    AutoWah() = default;
    struct Parameters
    {
        bool enabled = true;
        float sensitivity = 0.58f; // 0..1
        float startHz = 220.0f;
        float rangeOctaves = 3.4f;
        float resonance = 5.5f;
        float attackMs = 6.0f;
        float releaseMs = 140.0f;
    };

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = std::max (8000.0, newSampleRate);
        states.assign (static_cast<size_t> (std::max (1, numChannels)), {});
        reset();
    }

    void reset()
    {
        for (auto& s : states)
            s = {};

        envelope = 0.0f;
        smoothedCutoff = 200.0f;
        coefficientCountdown = 0;
        wasEnabled = false;
        lastCutoffHz.store (smoothedCutoff, std::memory_order_relaxed);
    }

    void process (juce::AudioBuffer<float>& buffer, const Parameters& p)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        if (numChannels == 0 || numSamples == 0)
            return;

        if (! p.enabled)
        {
            envelope = 0.0f;
            wasEnabled = false;
            return;
        }

        if (! wasEnabled)
        {
            for (auto& s : states)
                s = {};

            wasEnabled = true;
        }

        const auto releaseMs = std::max (1.0f, p.releaseMs);
        const auto attackMs = std::max (0.1f, p.attackMs);

        const float attackCoeff = 1.0f - std::exp (-1.0f / (0.001f * attackMs * static_cast<float> (sampleRate)));
        const float releaseCoeff = 1.0f - std::exp (-1.0f / (0.001f * releaseMs * static_cast<float> (sampleRate)));
        const float cutoffSmoothCoeff = 1.0f - std::exp (-1.0f / (0.004f * static_cast<float> (sampleRate)));

        const float sensGain = juce::Decibels::decibelsToGain (-24.0f + 48.0f * juce::jlimit (0.0f, 1.0f, p.sensitivity));
        const float start = juce::jlimit (20.0f, 4000.0f, p.startHz);
        const float maxCutoff = juce::jlimit (start, static_cast<float> (sampleRate * 0.45), start * std::pow (2.0f, juce::jlimit (0.0f, 6.0f, p.rangeOctaves)));
        const float resonance = juce::jlimit (0.5f, 20.0f, p.resonance);
        const float damping = 1.0f / resonance;

        float g = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

        auto updateCoefficients = [&] (float cutoff)
        {
            const float safeCutoff = juce::jlimit (20.0f, static_cast<float> (sampleRate * 0.45), cutoff);
            g = std::tan (juce::MathConstants<float>::pi * safeCutoff / static_cast<float> (sampleRate));
            const float denom = 1.0f + g * (g + damping);
            a1 = 1.0f / denom;
            a2 = g * a1;
            a3 = g * a2;
        };

        for (size_t ch = 1; ch < states.size(); ++ch)
            if (static_cast<int> (ch) >= numChannels)
                states[ch] = {};

        updateCoefficients (smoothedCutoff);

        for (int i = 0; i < numSamples; ++i)
        {
            float detector = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                detector = std::max (detector, std::abs (buffer.getReadPointer (ch)[i]));

            const float coeff = detector > envelope ? attackCoeff : releaseCoeff;
            envelope += coeff * (detector - envelope);

            const float levelDb = juce::Decibels::gainToDecibels (envelope * sensGain + 1.0e-9f, -90.0f);
            float normalised = juce::jlimit (0.0f, 1.0f, (levelDb + 54.0f) / 54.0f);
            normalised = std::pow (normalised, 0.75f);

            const float target = start * std::pow (maxCutoff / start, normalised);
            smoothedCutoff += cutoffSmoothCoeff * (target - smoothedCutoff);

            if (--coefficientCountdown <= 0)
            {
                updateCoefficients (smoothedCutoff);
                coefficientCountdown = 16;
            }

            const float makeup = 0.9f / std::sqrt (resonance);

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& s = states[static_cast<size_t> (ch)];
                const float input = buffer.getReadPointer (ch)[i];

                const float v3 = input - s.ic2eq;
                const float v1 = a1 * s.ic1eq + a2 * v3;
                const float v2 = s.ic2eq + a2 * s.ic1eq + a3 * v3;

                s.ic1eq = 2.0f * v1 - s.ic1eq;
                s.ic2eq = 2.0f * v2 - s.ic2eq;

                const float output = v1 * makeup;
                buffer.getWritePointer (ch)[i] = std::isfinite (output) ? output : 0.0f;
            }
        }

        lastCutoffHz.store (smoothedCutoff, std::memory_order_relaxed);
    }

    float getCurrentCutoffHz() const noexcept
    {
        return lastCutoffHz.load (std::memory_order_relaxed);
    }

private:
    struct ChannelState
    {
        float ic1eq = 0.0f;
        float ic2eq = 0.0f;
    };

    double sampleRate = 48000.0;
    std::vector<ChannelState> states;
    float envelope = 0.0f;
    float smoothedCutoff = 200.0f;
    int coefficientCountdown = 0;
    bool wasEnabled = false;
    std::atomic<float> lastCutoffHz { 200.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoWah)
};

} // namespace cerberus
