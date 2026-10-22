#include "PluginProcessor.h"

#include "Parameters.h"
#include "PluginEditor.h"
#include "Presets.h"

#include <algorithm>
#include <cmath>

namespace
{
using APVTS = juce::AudioProcessorValueTreeState;

std::unique_ptr<juce::AudioParameterFloat> makeFloat (const char* id,
                                                     const char* name,
                                                     juce::NormalisableRange<float> range,
                                                     float defaultValue,
                                                     const char* label)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 },
        name,
        range,
        defaultValue,
        juce::AudioParameterFloatAttributes().withLabel (label));
}

std::unique_ptr<juce::AudioParameterBool> makeBool (const char* id,
                                                    const char* name,
                                                    bool defaultValue)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, defaultValue);
}
} // namespace

CerberusAudioProcessor::CerberusAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    inputGainParam = apvts.getRawParameterValue (cerberus::params::inputGain);
    outputGainParam = apvts.getRawParameterValue (cerberus::params::outputGain);

    wahOnParam = apvts.getRawParameterValue (cerberus::params::wahOn);
    wahSensitivityParam = apvts.getRawParameterValue (cerberus::params::wahSensitivity);
    wahStartParam = apvts.getRawParameterValue (cerberus::params::wahStart);
    wahRangeParam = apvts.getRawParameterValue (cerberus::params::wahRange);
    wahResonanceParam = apvts.getRawParameterValue (cerberus::params::wahResonance);
    wahAttackParam = apvts.getRawParameterValue (cerberus::params::wahAttack);
    wahReleaseParam = apvts.getRawParameterValue (cerberus::params::wahRelease);

    compOnParam = apvts.getRawParameterValue (cerberus::params::compOn);
    compThresholdParam = apvts.getRawParameterValue (cerberus::params::compThreshold);
    compRatioParam = apvts.getRawParameterValue (cerberus::params::compRatio);
    compAttackParam = apvts.getRawParameterValue (cerberus::params::compAttack);
    compReleaseParam = apvts.getRawParameterValue (cerberus::params::compRelease);
    compMakeupParam = apvts.getRawParameterValue (cerberus::params::compMakeup);
    compMixParam = apvts.getRawParameterValue (cerberus::params::compMix);

    distOnParam = apvts.getRawParameterValue (cerberus::params::distOn);
    distDriveParam = apvts.getRawParameterValue (cerberus::params::distDrive);
    distToneParam = apvts.getRawParameterValue (cerberus::params::distTone);
    distMixParam = apvts.getRawParameterValue (cerberus::params::distMix);
    distOutputParam = apvts.getRawParameterValue (cerberus::params::distOutput);
    distModeParam = apvts.getRawParameterValue (cerberus::params::distMode);

    for (const auto* parameterID : cerberus::params::allParameterIDs)
        apvts.addParameterListener (parameterID, this);

    currentPreset.store (0, std::memory_order_relaxed);
    presetDirty.store (false, std::memory_order_relaxed);
}

CerberusAudioProcessor::~CerberusAudioProcessor()
{
    for (const auto* parameterID : cerberus::params::allParameterIDs)
        apvts.removeParameterListener (parameterID, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout CerberusAudioProcessor::createParameterLayout()
{
    using namespace cerberus::params;

    APVTS::ParameterLayout layout;

    auto global = std::make_unique<juce::AudioProcessorParameterGroup> ("global", "Global", " | ");
    global->addChild (makeFloat (inputGain, "Input", { -24.0f, 24.0f, 0.01f }, 0.0f, "dB"));
    global->addChild (makeFloat (outputGain, "Output", { -24.0f, 24.0f, 0.01f }, 0.0f, "dB"));
    layout.add (std::move (global));

    auto wah = std::make_unique<juce::AudioProcessorParameterGroup> ("autowah", "Auto-Wah", " | ");
    wah->addChild (makeBool (wahOn, "Auto-Wah On", true));
    wah->addChild (makeFloat (wahSensitivity, "Sensitivity", { 0.0f, 1.0f, 0.001f }, 0.58f, "%"));
    wah->addChild (makeFloat (wahStart, "Start Frequency", { 80.0f, 1600.0f, 1.0f, 0.45f }, 220.0f, "Hz"));
    wah->addChild (makeFloat (wahRange, "Sweep Range", { 0.5f, 5.0f, 0.01f }, 3.4f, "oct"));
    wah->addChild (makeFloat (wahResonance, "Resonance", { 0.5f, 12.0f, 0.01f, 0.7f }, 5.5f, ""));
    wah->addChild (makeFloat (wahAttack, "Envelope Attack", { 0.5f, 100.0f, 0.01f, 0.3f }, 6.0f, "ms"));
    wah->addChild (makeFloat (wahRelease, "Envelope Release", { 20.0f, 1000.0f, 0.1f, 0.35f }, 140.0f, "ms"));
    layout.add (std::move (wah));

    auto comp = std::make_unique<juce::AudioProcessorParameterGroup> ("compressor", "Compressor", " | ");
    comp->addChild (makeBool (compOn, "Compressor On", true));
    comp->addChild (makeFloat (compThreshold, "Threshold", { -60.0f, 0.0f, 0.01f }, -24.0f, "dB"));
    comp->addChild (makeFloat (compRatio, "Ratio", { 1.0f, 20.0f, 0.01f, 0.6f }, 4.0f, ":1"));
    comp->addChild (makeFloat (compAttack, "Attack", { 0.1f, 100.0f, 0.01f, 0.3f }, 8.0f, "ms"));
    comp->addChild (makeFloat (compRelease, "Release", { 20.0f, 1000.0f, 0.1f, 0.35f }, 120.0f, "ms"));
    comp->addChild (makeFloat (compMakeup, "Make-up Gain", { 0.0f, 24.0f, 0.01f }, 4.0f, "dB"));
    comp->addChild (makeFloat (compMix, "Parallel Mix", { 0.0f, 1.0f, 0.001f }, 1.0f, "%"));
    layout.add (std::move (comp));

    auto dist = std::make_unique<juce::AudioProcessorParameterGroup> ("distortion", "Distortion", " | ");
    dist->addChild (makeBool (distOn, "Distortion On", true));
    dist->addChild (makeFloat (distDrive, "Drive", { 0.0f, 36.0f, 0.01f, 0.8f }, 6.0f, "dB"));
    dist->addChild (makeFloat (distTone, "Tone", { 0.0f, 1.0f, 0.001f }, 0.70f, "%"));
    dist->addChild (makeFloat (distMix, "Mix", { 0.0f, 1.0f, 0.001f }, 0.55f, "%"));
    dist->addChild (makeFloat (distOutput, "Output", { -18.0f, 18.0f, 0.01f }, 0.0f, "dB"));
    dist->addChild (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { distMode, 1 },
        "Character",
        juce::StringArray { "Warm", "Crunch", "Fuzz" },
        0));
    layout.add (std::move (dist));

    return layout;
}

void CerberusAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = std::max (8000.0, sampleRate);
    preparedChannels = juce::jlimit (1, 2, juce::jmax (1, juce::jmax (getTotalNumInputChannels(),
                                                                      getTotalNumOutputChannels())));

    autoWah.prepare (currentSampleRate, preparedChannels);
    compressor.prepare (currentSampleRate, preparedChannels);
    distortion.prepare (currentSampleRate, std::max (1, samplesPerBlock), preparedChannels);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (std::max (1, samplesPerBlock));
    spec.numChannels = static_cast<juce::uint32> (preparedChannels);

    inputGainDsp.prepare (spec);
    outputGainDsp.prepare (spec);
    inputGainDsp.setRampDurationSeconds (0.02);
    outputGainDsp.setRampDurationSeconds (0.02);
    inputGainDsp.setGainLinear (juce::Decibels::decibelsToGain (inputGainParam->load()));
    outputGainDsp.setGainLinear (juce::Decibels::decibelsToGain (outputGainParam->load()));

    setLatencySamples (distortion.getLatencySamples());

    inputLevel.store (0.0f, std::memory_order_relaxed);
    outputLevel.store (0.0f, std::memory_order_relaxed);
    outputLevelLeft.store (0.0f, std::memory_order_relaxed);
    outputLevelRight.store (0.0f, std::memory_order_relaxed);
}

void CerberusAudioProcessor::releaseResources()
{
    autoWah.reset();
    compressor.reset();
    distortion.reset();
    inputGainDsp.reset();
    outputGainDsp.reset();
}

void CerberusAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int ch = preparedChannels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples == 0)
        return;

    const int processChannels = std::min (preparedChannels, buffer.getNumChannels());

    if (processChannels <= 0)
        return;

    juce::AudioBuffer<float> active (buffer.getArrayOfWritePointers(), processChannels, numSamples);

    const float inputPeak = active.getMagnitude (0, numSamples);

    juce::dsp::AudioBlock<float> block (active);
    juce::dsp::ProcessContextReplacing<float> context (block);

    inputGainDsp.setGainLinear (juce::Decibels::decibelsToGain (inputGainParam->load()));
    inputGainDsp.process (context);

    cerberus::AutoWah::Parameters wahParameters;
    wahParameters.enabled = wahOnParam->load() > 0.5f;
    wahParameters.sensitivity = wahSensitivityParam->load();
    wahParameters.startHz = wahStartParam->load();
    wahParameters.rangeOctaves = wahRangeParam->load();
    wahParameters.resonance = wahResonanceParam->load();
    wahParameters.attackMs = wahAttackParam->load();
    wahParameters.releaseMs = wahReleaseParam->load();
    autoWah.process (active, wahParameters);

    cerberus::Compressor::Parameters compressorParameters;
    compressorParameters.enabled = compOnParam->load() > 0.5f;
    compressorParameters.thresholdDb = compThresholdParam->load();
    compressorParameters.ratio = compRatioParam->load();
    compressorParameters.attackMs = compAttackParam->load();
    compressorParameters.releaseMs = compReleaseParam->load();
    compressorParameters.makeupDb = compMakeupParam->load();
    compressorParameters.mix = compMixParam->load();
    compressor.process (active, compressorParameters);

    cerberus::Distortion::Parameters distortionParameters;
    distortionParameters.enabled = distOnParam->load() > 0.5f;
    distortionParameters.driveDb = distDriveParam->load();
    distortionParameters.tone = distToneParam->load();
    distortionParameters.mix = distMixParam->load();
    distortionParameters.outputDb = distOutputParam->load();
    distortionParameters.mode = static_cast<cerberus::Distortion::Mode> (
        juce::jlimit (0, 2, juce::roundToInt (distModeParam->load())));
    distortion.process (active, distortionParameters);

    outputGainDsp.setGainLinear (juce::Decibels::decibelsToGain (outputGainParam->load()));
    outputGainDsp.process (context);

    const float outputPeak = active.getMagnitude (0, numSamples);
    const float outputLeft = active.getMagnitude (0, 0, numSamples);
    const float outputRight = processChannels > 1 ? active.getMagnitude (1, 0, numSamples) : outputLeft;
    inputLevel.store (std::isfinite (inputPeak) ? inputPeak : 0.0f, std::memory_order_relaxed);
    outputLevel.store (std::isfinite (outputPeak) ? outputPeak : 0.0f, std::memory_order_relaxed);
    outputLevelLeft.store (std::isfinite (outputLeft) ? outputLeft : 0.0f, std::memory_order_relaxed);
    outputLevelRight.store (std::isfinite (outputRight) ? outputRight : 0.0f, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* CerberusAudioProcessor::createEditor()
{
    return new CerberusAudioProcessorEditor (*this);
}

int CerberusAudioProcessor::getNumPrograms()
{
    return cerberus::getNumFactoryPresets();
}

int CerberusAudioProcessor::getCurrentProgram()
{
    return currentPreset.load (std::memory_order_relaxed);
}

void CerberusAudioProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

const juce::String CerberusAudioProcessor::getProgramName (int index)
{
    return cerberus::getFactoryPresetName (index);
}

void CerberusAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void CerberusAudioProcessor::applyPreset (int index)
{
    const auto& presets = cerberus::getFactoryPresets();

    if (index < 0 || index >= static_cast<int> (presets.size()))
        return;

    applyingPreset.store (true, std::memory_order_relaxed);

    for (auto* parameter : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    for (const auto& value : presets[static_cast<size_t> (index)].overrides)
    {
        if (auto* parameter = apvts.getParameter (value.id))
        {
            const auto& range = parameter->getNormalisableRange();
            const float deNormalised = juce::jlimit (range.start, range.end, value.value);
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (deNormalised));
        }
    }

    currentPreset.store (index, std::memory_order_relaxed);
    presetDirty.store (false, std::memory_order_relaxed);
    applyingPreset.store (false, std::memory_order_relaxed);
}

void CerberusAudioProcessor::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset.load (std::memory_order_relaxed))
        presetDirty.store (true, std::memory_order_relaxed);
}

void CerberusAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", currentPreset.load (std::memory_order_relaxed), nullptr);
    state.setProperty ("presetDirty", presetDirty.load (std::memory_order_relaxed), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void CerberusAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);

    if (! tree.isValid())
        return;

    const int presetIndex = static_cast<int> (tree.getProperty ("presetIndex", 0));
    const bool dirty = static_cast<bool> (tree.getProperty ("presetDirty", false));

    applyingPreset.store (true, std::memory_order_relaxed);
    apvts.replaceState (tree);
    applyingPreset.store (false, std::memory_order_relaxed);

    currentPreset.store (juce::jlimit (0, cerberus::getNumFactoryPresets() - 1, presetIndex),
                         std::memory_order_relaxed);
    presetDirty.store (dirty, std::memory_order_relaxed);
}

bool CerberusAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::mono()
        || mainIn == juce::AudioChannelSet::stereo();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CerberusAudioProcessor();
}
