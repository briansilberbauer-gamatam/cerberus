#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "dsp/AutoWah.h"
#include "dsp/Compressor.h"
#include "dsp/Distortion.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

class CerberusAudioProcessor final : public juce::AudioProcessor,
                                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    CerberusAudioProcessor();
    ~CerberusAudioProcessor() override;

    using juce::AudioProcessor::processBlock;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Cerberus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return apvts; }

    int getCurrentPresetIndex() const noexcept { return currentPreset.load (std::memory_order_relaxed); }
    bool isPresetDirty() const noexcept { return presetDirty.load (std::memory_order_relaxed); }

    float getInputLevel() const noexcept { return inputLevel.load (std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return outputLevel.load (std::memory_order_relaxed); }
    float getOutputLevelLeft() const noexcept { return outputLevelLeft.load (std::memory_order_relaxed); }
    float getOutputLevelRight() const noexcept { return outputLevelRight.load (std::memory_order_relaxed); }
    float getCompressorGainReductionDb() const noexcept { return compressor.getGainReductionDb(); }
    float getWahCutoffHz() const noexcept { return autoWah.getCurrentCutoffHz(); }
    int getDistortionLatencySamples() const noexcept { return distortion.getLatencySamples(); }

    // Lock-free post-FX waveform telemetry for the editor visualizer.
    static constexpr int waveformSize = 2048;
    static constexpr int waveformDecimation = 8;

    uint32_t getWaveformWriteIndex() const noexcept
    {
        return waveformWriteIndex.load (std::memory_order_relaxed);
    }

    float getWaveformSample (uint32_t index) const noexcept
    {
        return waveformRing[index & (waveformSize - 1)].load (std::memory_order_relaxed);
    }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void applyPreset (int index);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    cerberus::AutoWah autoWah;
    cerberus::Compressor compressor;
    cerberus::Distortion distortion;

    juce::dsp::Gain<float> inputGainDsp;
    juce::dsp::Gain<float> outputGainDsp;

    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* wahOnParam = nullptr;
    std::atomic<float>* wahSensitivityParam = nullptr;
    std::atomic<float>* wahStartParam = nullptr;
    std::atomic<float>* wahRangeParam = nullptr;
    std::atomic<float>* wahResonanceParam = nullptr;
    std::atomic<float>* wahAttackParam = nullptr;
    std::atomic<float>* wahReleaseParam = nullptr;
    std::atomic<float>* compOnParam = nullptr;
    std::atomic<float>* compThresholdParam = nullptr;
    std::atomic<float>* compRatioParam = nullptr;
    std::atomic<float>* compAttackParam = nullptr;
    std::atomic<float>* compReleaseParam = nullptr;
    std::atomic<float>* compMakeupParam = nullptr;
    std::atomic<float>* compMixParam = nullptr;
    std::atomic<float>* distOnParam = nullptr;
    std::atomic<float>* distDriveParam = nullptr;
    std::atomic<float>* distToneParam = nullptr;
    std::atomic<float>* distMixParam = nullptr;
    std::atomic<float>* distOutputParam = nullptr;
    std::atomic<float>* distModeParam = nullptr;

    std::atomic<int> currentPreset { 0 };
    std::atomic<bool> presetDirty { false };
    std::atomic<bool> applyingPreset { false };

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> outputLevelLeft { 0.0f };
    std::atomic<float> outputLevelRight { 0.0f };

    std::array<std::atomic<float>, waveformSize> waveformRing {};
    std::atomic<uint32_t> waveformWriteIndex { 0 };

    double currentSampleRate = 48000.0;
    int preparedChannels = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusAudioProcessor)
};
