#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <vector>

namespace cerberus
{

/**
    A stereo-linked, feed-forward peak compressor with a fixed 6 dB soft knee.
    Manual make-up gain and parallel mix are included. The detector runs on the
    linked maximum of all channels so the stereo image does not wander.
*/
class Compressor
{
public:
    Compressor() = default;
    struct Parameters
    {
        bool enabled = true;
        float thresholdDb = -24.0f;
        float ratio = 4.0f;
        float attackMs = 8.0f;
        float releaseMs = 120.0f;
        float makeupDb = 4.0f;
        float mix = 1.0f;
    };

    void prepare (double newSampleRate, int numChannels)
    {
        sampleRate = std::max (8000.0, newSampleRate);
        numChannelsPrepared = std::max (1, numChannels);
        reset();
    }

    void reset()
    {
        envelope = 0.0f;
        wasEnabled = false;
        lastGainReductionDb.store (0.0f, std::memory_order_relaxed);
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
            lastGainReductionDb.store (0.0f, std::memory_order_relaxed);
            return;
        }

        if (! wasEnabled)
        {
            envelope = 0.0f;
            wasEnabled = true;
        }

        const float threshold = juce::jlimit (-72.0f, 6.0f, p.thresholdDb);
        const float ratio = juce::jlimit (1.0f, 40.0f, p.ratio);
        const float attackMs = std::max (0.05f, p.attackMs);
        const float releaseMs = std::max (1.0f, p.releaseMs);
        const float makeup = juce::Decibels::decibelsToGain (p.makeupDb);
        const float mix = juce::jlimit (0.0f, 1.0f, p.mix);

        const float attackCoeff = 1.0f - std::exp (-1.0f / (0.001f * attackMs * static_cast<float> (sampleRate)));
        const float releaseCoeff = 1.0f - std::exp (-1.0f / (0.001f * releaseMs * static_cast<float> (sampleRate)));

        constexpr float kneeDb = 6.0f;
        const float slope = 1.0f - 1.0f / ratio;

        float maxReduction = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float detector = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                detector = std::max (detector, std::abs (buffer.getReadPointer (ch)[i]));

            const float coeff = detector > envelope ? attackCoeff : releaseCoeff;
            envelope += coeff * (detector - envelope);

            const float inputDb = juce::Decibels::gainToDecibels (envelope + 1.0e-10f, -120.0f);
            const float over = inputDb - threshold;

            float reductionDb = 0.0f;

            if (over > kneeDb * 0.5f)
            {
                reductionDb = -over * slope;
            }
            else if (over > -kneeDb * 0.5f)
            {
                const float kneePosition = over + kneeDb * 0.5f;
                reductionDb = -slope * kneePosition * kneePosition / (2.0f * kneeDb);
            }

            maxReduction = std::max (maxReduction, -reductionDb);

            const float gain = juce::Decibels::decibelsToGain (reductionDb) * makeup;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float dry = buffer.getReadPointer (ch)[i];
                const float wet = dry * gain;
                const float output = dry + mix * (wet - dry);
                buffer.getWritePointer (ch)[i] = std::isfinite (output) ? output : 0.0f;
            }
        }

        lastGainReductionDb.store (maxReduction, std::memory_order_relaxed);
    }

    float getGainReductionDb() const noexcept
    {
        return lastGainReductionDb.load (std::memory_order_relaxed);
    }

private:
    double sampleRate = 48000.0;
    int numChannelsPrepared = 2;
    float envelope = 0.0f;
    bool wasEnabled = false;
    std::atomic<float> lastGainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Compressor)
};

} // namespace cerberus
