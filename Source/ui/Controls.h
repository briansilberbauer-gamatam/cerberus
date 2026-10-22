#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "Theme.h"

#include <functional>
#include <memory>
#include <vector>

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

class PowerSwitch final : public juce::Button
{
public:
    PowerSwitch (juce::AudioProcessorValueTreeState& state,
                 const juce::String& parameterID,
                 juce::Colour accent,
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
    juce::Colour accentColour;
    bool on = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerSwitch)
};

class ModeBar final : public juce::Component
{
public:
    ModeBar (juce::AudioProcessorValueTreeState& state,
             const juce::String& parameterID,
             juce::Colour accent);

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
    juce::Colour accentColour;
    int selected = 0;
    int hovered = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModeBar)
};

class GainReductionMeter final : public juce::Component
{
public:
    explicit GainReductionMeter (juce::Colour accent);

    void setReductionDb (float newReductionDb);
    void paint (juce::Graphics&) override;

private:
    juce::Colour accentColour;
    float displayedDb = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};

class SweepMeter final : public juce::Component
{
public:
    explicit SweepMeter (juce::Colour accent);

    void setCutoffHz (float newCutoffHz);
    void paint (juce::Graphics&) override;

private:
    juce::Colour accentColour;
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

} // namespace cerberus::ui
