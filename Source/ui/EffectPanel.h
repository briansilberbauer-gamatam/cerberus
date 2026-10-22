#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Controls.h"

#include <memory>

namespace cerberus::ui
{

class EffectPanel final : public juce::Component
{
public:
    EffectPanel (juce::AudioProcessorValueTreeState& state,
                 juce::String number,
                 juce::String title,
                 juce::String subtitle,
                 juce::Colour accent,
                 const juce::String& powerParameterID,
                 int knobColumns);

    ~EffectPanel() override;

    ParamKnob* addKnob (const juce::String& displayName,
                        const juce::String& parameterID,
                        ParamKnob::Formatter formatter);

    void setBottomComponent (std::unique_ptr<juce::Component> component, int preferredHeight);
    void setEffectEnabled (bool shouldBeEnabled);

    bool isEffectEnabled() const noexcept { return effectEnabled; }
    juce::Colour getAccent() const noexcept { return accentColour; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String panelNumber;
    juce::String panelTitle;
    juce::String panelSubtitle;
    juce::Colour accentColour;
    int columns = 3;
    bool effectEnabled = true;

    std::unique_ptr<PowerSwitch> powerSwitch;
    juce::OwnedArray<ParamKnob> knobs;
    std::unique_ptr<juce::Component> bottomComponent;
    int bottomHeight = 0;

    juce::Rectangle<int> getKnobArea() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectPanel)
};

} // namespace cerberus::ui
