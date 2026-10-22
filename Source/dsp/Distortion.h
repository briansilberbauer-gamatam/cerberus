#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace cerberus
{

/**
    A four-times oversampled waveshaper with three voicings and a post tone
    filter. The dry signal is mixed with the wet signal inside the oversampled
    domain, which keeps both paths phase-aligned without an extra delay line.

    When bypassed, the input is passed through a small delay equal to the
    oversampler latency. This keeps the plugin's reported latency constant while
    avoiding the cost of running the oversampler for a bypassed module.
*/
class Distortion
{
public:
    Distortion() = default;
    enum class Mode
    {
        warm = 0,
        crunch = 1,
        fuzz = 2
    };

    struct Parameters
    {
        bool enabled = true;
        float driveDb = 6.0f;
        float tone = 0.70f;
        float mix = 0.55f;
        float outputDb = 0.0f;
        Mode mode = Mode::warm;
    };

    void prepare (double newSampleRate, int maximumBlockSize, int numChannels)
    {
        sampleRate = std::max (8000.0, newSampleRate);
        channels = std::max (1, numChannels);
        maxBlock = std::max (1, maximumBlockSize);

        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
            static_cast<size_t> (channels),
            2,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true,
            true);

        oversampler->initProcessing (static_cast<size_t> (maxBlock));
        oversamplingFactor = static_cast<float> (oversampler->getOversamplingFactor());
        latencySamples = juce::jmax (0, juce::roundToInt (oversampler->getLatencyInSamples()));

        bypassDelay.assign (static_cast<size_t> (channels), std::vector<float> (static_cast<size_t> (latencySamples + 1), 0.0f));
        channelPointers.assign (static_cast<size_t> (channels), nullptr);
        bypassWritePosition = 0;

        reset();
    }

    void reset()
    {
        if (oversampler != nullptr)
            oversampler->reset();

        toneState.assign (static_cast<size_t> (channels), 0.0f);
        dcInput.assign (static_cast<size_t> (channels), 0.0f);
        dcOutput.assign (static_cast<size_t> (channels), 0.0f);

        for (auto& channel : bypassDelay)
            std::fill (channel.begin(), channel.end(), 0.0f);

        bypassWritePosition = 0;
    }

    void process (juce::AudioBuffer<float>& buffer, const Parameters& p)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        if (numChannels == 0 || numSamples == 0)
            return;

        if (! p.enabled)
        {
            processBypassDelay (buffer);
            return;
        }

        if (oversampler == nullptr || buffer.getNumSamples() > maxBlock || numChannels > channels)
        {
            // A host should never exceed the prepared block size, but failing
            // closed here is preferable to reading outside an internal buffer.
            buffer.clear();
            return;
        }

        const float driveGain = juce::Decibels::decibelsToGain (juce::jlimit (-12.0f, 48.0f, p.driveDb));
        const float driveCompensation = 1.0f / (1.0f + 0.06f * driveGain);
        const float outputGain = juce::Decibels::decibelsToGain (juce::jlimit (-36.0f, 24.0f, p.outputDb));
        const float mix = juce::jlimit (0.0f, 1.0f, p.mix);

        const float tone = juce::jlimit (0.0f, 1.0f, p.tone);
        const float toneHz = juce::jlimit (600.0f, static_cast<float> (sampleRate) * 0.45f * oversamplingFactor,
                                           1000.0f * std::pow (18.0f, tone));
        const float toneCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * toneHz
                                                    / static_cast<float> (sampleRate * oversamplingFactor));

        auto block = juce::dsp::AudioBlock<float> (buffer);
        auto oversampled = oversampler->processSamplesUp (juce::dsp::AudioBlock<const float> (block));

        const int oversampledSamples = static_cast<int> (oversampled.getNumSamples());

        for (int ch = 0; ch < numChannels; ++ch)
            channelPointers[static_cast<size_t> (ch)] = oversampled.getChannelPointer (static_cast<size_t> (ch));

        for (int i = 0; i < oversampledSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = channelPointers[static_cast<size_t> (ch)];
                const float dry = data[i];

                float shaped = 0.0f;
                const float driven = dry * driveGain;

                switch (p.mode)
                {
                    case Mode::warm:
                        shaped = std::tanh (driven);
                        break;

                    case Mode::crunch:
                        shaped = std::tanh (1.6f * driven + 0.35f) - 0.33637556f;
                        break;

                    case Mode::fuzz:
                    {
                        const float fuzz = std::tanh (2.6f * driven + 0.80f) - 0.66403677f;
                        shaped = juce::jlimit (-1.0f, 1.0f, fuzz * 1.25f);
                        break;
                    }
                }

                float wet = shaped * driveCompensation;

                auto& toneValue = toneState[static_cast<size_t> (ch)];
                toneValue += toneCoeff * (wet - toneValue);
                wet = toneValue;

                auto& previousInput = dcInput[static_cast<size_t> (ch)];
                auto& previousOutput = dcOutput[static_cast<size_t> (ch)];
                const float dcBlocked = wet - previousInput + 0.995f * previousOutput;
                previousInput = wet;
                previousOutput = std::isfinite (dcBlocked) ? dcBlocked : 0.0f;
                wet = previousOutput * outputGain;

                const float output = dry + mix * (wet - dry);
                data[i] = std::isfinite (output) ? output : 0.0f;
            }
        }

        oversampler->processSamplesDown (block);
    }

    int getLatencySamples() const noexcept
    {
        return latencySamples;
    }

private:
    void processBypassDelay (juce::AudioBuffer<float>& buffer)
    {
        if (latencySamples <= 0)
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        const int delaySize = static_cast<int> (bypassDelay.front().size());

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto& delay = bypassDelay[static_cast<size_t> (ch)];
                int readPosition = bypassWritePosition - latencySamples;
                if (readPosition < 0)
                    readPosition += delaySize;

                const float input = buffer.getReadPointer (ch)[i];
                const float delayed = delay[static_cast<size_t> (readPosition)];
                delay[static_cast<size_t> (bypassWritePosition)] = input;
                buffer.getWritePointer (ch)[i] = delayed;
            }

            if (++bypassWritePosition >= delaySize)
                bypassWritePosition = 0;
        }
    }

    double sampleRate = 48000.0;
    int channels = 2;
    int maxBlock = 512;
    int latencySamples = 0;
    int bypassWritePosition = 0;
    float oversamplingFactor = 4.0f;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    std::vector<std::vector<float>> bypassDelay;
    std::vector<float*> channelPointers;
    std::vector<float> toneState;
    std::vector<float> dcInput;
    std::vector<float> dcOutput;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Distortion)
};

} // namespace cerberus
