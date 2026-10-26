#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"

#include <functional>
#include <memory>
#include <vector>

class CerberusAudioProcessor;

namespace cerberus::ui
{

class ParamKnob final : public juce::Component
{
public:
    using Formatter = std::function<juce::String (float)>;

    ParamKnob (juce::AudioProcessorValueTreeState& state,
               const juce::String& parameterID,
               const juce::String& displayName,
               Formatter formatter,
               juce::Colour accent);

    void setAccent (juce::Colour newAccent);
    void resized() override;

private:
    void updateValueLabel();

    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    Formatter valueFormatter;
    juce::Colour accentColour;
    juce::RangedAudioParameter* parameter = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamKnob)
};

class ParamSlider final : public juce::Component
{
public:
    using Formatter = std::function<juce::String (float)>;

    ParamSlider (juce::AudioProcessorValueTreeState& state,
                 const juce::String& parameterID,
                 const juce::String& displayName,
                 Formatter formatter);

    void resized() override;

private:
    void updateValueLabel();

    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    Formatter valueFormatter;
    juce::RangedAudioParameter* parameter = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamSlider)
};

class PowerSwitch final : public juce::Button
{
public:
    PowerSwitch (juce::AudioProcessorValueTreeState& state,
                 const juce::String& parameterID,
                 juce::String buttonLabel,
                 std::function<void (bool)> onChanged = {});

    using juce::Button::clicked;

    bool isOn() const noexcept { return on; }
    void paintButton (juce::Graphics&, bool isMouseOverButton, bool isButtonDown) override;
    void clicked() override;

private:
    void setOn (bool shouldBeOn);

    juce::RangedAudioParameter* parameter = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    std::function<void (bool)> changeCallback;
    juce::String label;
    bool on = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerSwitch)
};

class ModeBar final : public juce::Component
{
public:
    ModeBar (juce::AudioProcessorValueTreeState& state,
             const juce::String& parameterID);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> segmentBounds (int index) const;
    void setSelected (int newIndex);
    int indexAt (juce::Point<float> position) const;

    juce::RangedAudioParameter* parameter = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    juce::StringArray choices;
    int selected = 0;
    int hovered = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeBar)
};

class GainReductionMeter final : public juce::Component
{
public:
    GainReductionMeter();

    void setReductionDb (float newReductionDb);
    void paint (juce::Graphics&) override;

private:
    float displayedDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};

class SweepMeter final : public juce::Component
{
public:
    SweepMeter();

    void setCutoffHz (float newCutoffHz);
    void paint (juce::Graphics&) override;

private:
    float displayedHz = 220.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SweepMeter)
};

class StereoMeter final : public juce::Component,
                          public juce::SettableTooltipClient
{
public:
    StereoMeter();

    void setLevels (float left, float right);
    void mouseDown (const juce::MouseEvent&) override;
    void paint (juce::Graphics&) override;

private:
    static float levelToNormalised (float level);
    void paintBar (juce::Graphics&, juce::Rectangle<float> bounds, float level, float peak) const;

    float displayLeft = 0.0f;
    float displayRight = 0.0f;
    float peakLeft = 0.0f;
    float peakRight = 0.0f;
    int peakHoldTicks = 0;
    int clipTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoMeter)
};

class PillNavButton final : public juce::Button
{
public:
    explicit PillNavButton (bool pointsLeft);

    void paintButton (juce::Graphics&, bool isMouseOverButton, bool isButtonDown) override;

private:
    bool left;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PillNavButton)
};

class PresetPill final : public juce::Component
{
public:
    explicit PresetPill (CerberusAudioProcessor&);

    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void stepPreset (int direction);

    CerberusAudioProcessor& processor;
    PillNavButton prevButton { true };
    PillNavButton nextButton { false };
    juce::ComboBox presetBox;
    int lastPreset = 0;
    bool lastDirty = false;
    bool syncing = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPill)
};

} // namespace cerberus::ui
