#include "dsp/AutoWah.h"
#include "dsp/Compressor.h"
#include "dsp/Distortion.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void check (bool condition, const std::string& message)
{
    if (condition)
    {
        std::cout << "  PASS  " << message << '\n';
    }
    else
    {
        std::cout << "  FAIL  " << message << '\n';
        ++failures;
    }
}

void fillSine (juce::AudioBuffer<float>& buffer, double sampleRate, float frequency, float amplitude)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer (ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            samples[i] = amplitude * std::sin (juce::MathConstants<float>::twoPi * frequency
                                               * static_cast<float> (i) / static_cast<float> (sampleRate));
    }
}

float rms (const juce::AudioBuffer<float>& buffer, int startSample)
{
    double sum = 0.0;
    int count = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* samples = buffer.getReadPointer (ch);

        for (int i = startSample; i < buffer.getNumSamples(); ++i)
        {
            sum += static_cast<double> (samples[i]) * static_cast<double> (samples[i]);
            ++count;
        }
    }

    return count > 0 ? static_cast<float> (std::sqrt (sum / static_cast<double> (count))) : 0.0f;
}

bool allFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (buffer.getReadPointer (ch)[i]))
                return false;

    return true;
}

void testAutoWah()
{
    std::cout << "\nAutoWah\n";

    constexpr double sampleRate = 48000.0;
    constexpr int numSamples = 48000;

    cerberus::AutoWah wah;
    wah.prepare (sampleRate, 2);

    cerberus::AutoWah::Parameters params;
    params.enabled = true;
    params.sensitivity = 0.65f;
    params.startHz = 220.0f;
    params.rangeOctaves = 3.5f;
    params.resonance = 5.0f;
    params.attackMs = 3.0f;
    params.releaseMs = 80.0f;

    juce::AudioBuffer<float> quiet (2, numSamples);
    fillSine (quiet, sampleRate, 220.0f, 0.001f);
    wah.process (quiet, params);
    const float quietCutoff = wah.getCurrentCutoffHz();

    wah.reset();
    juce::AudioBuffer<float> loud (2, numSamples);
    fillSine (loud, sampleRate, 220.0f, 0.7f);
    wah.process (loud, params);
    const float loudCutoff = wah.getCurrentCutoffHz();

    check (allFinite (loud), "output remains finite at high input level");
    check (loudCutoff > quietCutoff * 2.5f,
           "envelope opens the filter (quiet " + std::to_string (quietCutoff)
               + " Hz, loud " + std::to_string (loudCutoff) + " Hz)");

    juce::AudioBuffer<float> silence (2, 24000);
    silence.clear();
    wah.process (silence, params);
    check (wah.getCurrentCutoffHz() < loudCutoff * 0.8f,
           "filter closes again on release");

    wah.reset();
    params.startHz = 1600.0f;
    params.rangeOctaves = 5.0f;
    params.resonance = 12.0f;
    juce::AudioBuffer<float> hot (2, 2048);
    fillSine (hot, sampleRate, 82.4f, 1.5f);
    wah.process (hot, params);
    check (allFinite (hot), "extreme resonance and input gain stay stable");
}

void testCompressor()
{
    std::cout << "\nCompressor\n";

    constexpr double sampleRate = 48000.0;
    constexpr int numSamples = 48000;

    cerberus::Compressor compressor;
    compressor.prepare (sampleRate, 2);

    cerberus::Compressor::Parameters params;
    params.enabled = true;
    params.thresholdDb = -20.0f;
    params.ratio = 8.0f;
    params.attackMs = 3.0f;
    params.releaseMs = 100.0f;
    params.makeupDb = 0.0f;
    params.mix = 1.0f;

    juce::AudioBuffer<float> loud (2, numSamples);
    fillSine (loud, sampleRate, 440.0f, 0.5f);
    const float inputRms = rms (loud, 0);
    compressor.process (loud, params);
    const float outputRms = rms (loud, numSamples / 2);
    const float reductionDb = juce::Decibels::gainToDecibels (outputRms / inputRms);
    const float reportedReduction = compressor.getGainReductionDb();

    check (allFinite (loud), "output remains finite");
    check (reductionDb < -6.0f,
           "heavy input is compressed (RMS change " + std::to_string (reductionDb) + " dB)");
    check (reportedReduction > 6.0f,
           "gain-reduction meter reports real reduction (" + std::to_string (reportedReduction) + " dB)");

    cerberus::Compressor quietCompressor;
    quietCompressor.prepare (sampleRate, 2);
    juce::AudioBuffer<float> quiet (2, numSamples);
    fillSine (quiet, sampleRate, 440.0f, 0.01f);
    const float quietInputRms = rms (quiet, 0);
    quietCompressor.process (quiet, params);
    const float quietOutputRms = rms (quiet, numSamples / 2);
    const float quietChangeDb = juce::Decibels::gainToDecibels (quietOutputRms / quietInputRms);

    check (std::abs (quietChangeDb) < 1.0f,
           "signal below threshold passes at unity (" + std::to_string (quietChangeDb) + " dB)");
}

void testDistortion()
{
    std::cout << "\nDistortion\n";

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;
    constexpr int totalSamples = 48000;

    cerberus::Distortion distortion;
    distortion.prepare (sampleRate, blockSize, 2);

    cerberus::Distortion::Parameters params;
    params.enabled = true;
    params.driveDb = 24.0f;
    params.tone = 0.85f;
    params.mix = 1.0f;
    params.outputDb = 0.0f;
    params.mode = cerberus::Distortion::Mode::warm;

    juce::AudioBuffer<float> signal (2, totalSamples);
    fillSine (signal, sampleRate, 440.0f, 0.2f);

    int position = 0;
    while (position < totalSamples)
    {
        const int blockLength = std::min (blockSize, totalSamples - position);
        juce::AudioBuffer<float> block (2, blockLength);

        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom (ch, 0, signal, ch, position, blockLength);

        distortion.process (block, params);

        for (int ch = 0; ch < 2; ++ch)
            signal.copyFrom (ch, position, block, ch, 0, blockLength);

        position += blockLength;
    }

    check (allFinite (signal), "output remains finite at 24 dB drive");
    check (distortion.getLatencySamples() > 0, "oversampler reports latency");

    const int analysisStart = 8192;
    const int analysisLength = 8192;
    double sumSin = 0.0, sumCos = 0.0, sumSquares = 0.0, sum = 0.0;
    int count = 0;

    for (int ch = 0; ch < 2; ++ch)
    {
        const auto* samples = signal.getReadPointer (ch);

        for (int i = analysisStart; i < analysisStart + analysisLength; ++i)
        {
            const double phase = juce::MathConstants<double>::twoPi * 440.0 * static_cast<double> (i) / sampleRate;
            const double x = samples[i];
            sumSin += x * std::sin (phase);
            sumCos += x * std::cos (phase);
            sumSquares += x * x;
            sum += x;
            ++count;
        }
    }

    const double a = 2.0 * sumSin / count;
    const double b = 2.0 * sumCos / count;
    const double fundamentalPower = 0.5 * (a * a + b * b);
    const double totalPower = sumSquares / count;
    const double residualPower = std::max (0.0, totalPower - fundamentalPower);
    const double thd = std::sqrt (residualPower / std::max (1.0e-12, fundamentalPower));
    const double dc = sum / count;

    check (thd > 0.10, "waveshaper adds harmonics (THD " + std::to_string (thd * 100.0) + "%)");
    check (std::abs (dc) < 0.01, "DC blocker removes offset (DC " + std::to_string (dc) + ")");

    cerberus::Distortion bypass;
    bypass.prepare (sampleRate, blockSize, 2);
    const int latency = bypass.getLatencySamples();

    juce::AudioBuffer<float> impulse (2, blockSize);
    impulse.clear();
    impulse.setSample (0, 100, 1.0f);
    impulse.setSample (1, 100, 1.0f);

    auto bypassParams = params;
    bypassParams.enabled = false;
    bypass.process (impulse, bypassParams);

    int peakIndex = 0;
    float peak = 0.0f;

    for (int i = 0; i < blockSize; ++i)
    {
        const float value = std::abs (impulse.getSample (0, i));

        if (value > peak)
        {
            peak = value;
            peakIndex = i;
        }
    }

    check (peakIndex == 100 + latency,
           "bypass path is delayed by exactly the reported latency ("
               + std::to_string (peakIndex) + " vs " + std::to_string (100 + latency) + ")");
}

void testProcessorStability()
{
    std::cout << "\nCombined stability\n";

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 256;

    cerberus::AutoWah wah;
    cerberus::Compressor compressor;
    cerberus::Distortion distortion;
    wah.prepare (sampleRate, 2);
    compressor.prepare (sampleRate, 2);
    distortion.prepare (sampleRate, blockSize, 2);

    cerberus::AutoWah::Parameters wahParams;
    wahParams.sensitivity = 1.0f;
    wahParams.startHz = 1000.0f;
    wahParams.rangeOctaves = 4.5f;
    wahParams.resonance = 10.0f;

    cerberus::Compressor::Parameters compParams;
    compParams.thresholdDb = -40.0f;
    compParams.ratio = 20.0f;
    compParams.makeupDb = 12.0f;

    cerberus::Distortion::Parameters distParams;
    distParams.driveDb = 36.0f;
    distParams.mix = 1.0f;
    distParams.outputDb = 6.0f;
    distParams.mode = cerberus::Distortion::Mode::fuzz;

    bool stable = true;
    float maximum = 0.0f;

    for (int block = 0; block < 200; ++block)
    {
        juce::AudioBuffer<float> buffer (2, blockSize);

        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float t = static_cast<float> ((block * blockSize + i)) / static_cast<float> (sampleRate);
                const float envelope = std::exp (-std::fmod (t, 1.0f) * 2.0f);
                buffer.setSample (ch, i, envelope * std::sin (juce::MathConstants<float>::twoPi * 110.0f * t));
            }
        }

        wah.process (buffer, wahParams);
        compressor.process (buffer, compParams);
        distortion.process (buffer, distParams);

        if (! allFinite (buffer))
            stable = false;

        maximum = std::max (maximum, buffer.getMagnitude (0, blockSize));
    }

    check (stable, "chaotic input under extreme settings remains finite");
    check (maximum < 20.0f, "extreme settings do not run away (peak " + std::to_string (maximum) + ")");
}

} // namespace

int main()
{
    std::cout << "Cerberus DSP tests\n";
    std::cout << "==================\n";

    testAutoWah();
    testCompressor();
    testDistortion();
    testProcessorStability();

    std::cout << "\n";
    if (failures == 0)
        std::cout << "All tests passed.\n";
    else
        std::cout << failures << " test(s) failed.\n";

    return failures == 0 ? 0 : 1;
}
