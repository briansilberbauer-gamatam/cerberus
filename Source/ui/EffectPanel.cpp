#include "EffectPanel.h"

#include <algorithm>
#include <cmath>

namespace cerberus::ui
{

EffectPanel::EffectPanel (juce::AudioProcessorValueTreeState& state,
                          juce::String number,
                          juce::String title,
                          juce::String subtitle,
                          juce::Colour accent,
                          const juce::String& powerParameterID,
                          int knobColumns)
    : apvts (state),
      panelNumber (std::move (number)),
      panelTitle (std::move (title)),
      panelSubtitle (std::move (subtitle)),
      accentColour (accent),
      columns (juce::jmax (1, knobColumns))
{
    powerSwitch = std::make_unique<PowerSwitch> (apvts, powerParameterID, accentColour,
                                                  [this] (bool on) { setEffectEnabled (on); });
    addAndMakeVisible (*powerSwitch);
    setEffectEnabled (powerSwitch->isOn());
}

EffectPanel::~EffectPanel() = default;

ParamKnob* EffectPanel::addKnob (const juce::String& displayName,
                                 const juce::String& parameterID,
                                 ParamKnob::Formatter formatter)
{
    auto* knob = knobs.add (new ParamKnob (apvts, parameterID, displayName,
                                           std::move (formatter), accentColour));
    addAndMakeVisible (*knob);
    knob->setAlpha (effectEnabled ? 1.0f : 0.38f);
    resized();
    return knob;
}

void EffectPanel::setBottomComponent (std::unique_ptr<juce::Component> component, int preferredHeight)
{
    if (bottomComponent != nullptr)
        removeChildComponent (bottomComponent.get());

    bottomComponent = std::move (component);
    bottomHeight = preferredHeight;

    if (bottomComponent != nullptr)
    {
        addAndMakeVisible (*bottomComponent);
        bottomComponent->setAlpha (effectEnabled ? 1.0f : 0.38f);
    }

    resized();
    repaint();
}

juce::Rectangle<int> EffectPanel::getKnobArea() const
{
    auto area = getLocalBounds().reduced (18, 0);
    const int bottomTop = bottomComponent != nullptr ? getHeight() - 18 - bottomHeight : getHeight() - 16;
    area.setTop (92);
    area.setBottom (juce::jmax (100, bottomTop - 8));
    return area;
}

void EffectPanel::setEffectEnabled (bool shouldBeEnabled)
{
    if (effectEnabled == shouldBeEnabled && knobs.size() > 0)
        return;

    effectEnabled = shouldBeEnabled;

    for (auto* knob : knobs)
        knob->setAlpha (effectEnabled ? 1.0f : 0.38f);

    if (bottomComponent != nullptr)
        bottomComponent->setAlpha (effectEnabled || dynamic_cast<ModeBar*> (bottomComponent.get()) == nullptr
                                       ? 1.0f
                                       : 0.38f);

    repaint();
}

void EffectPanel::resized()
{
    if (powerSwitch != nullptr)
        powerSwitch->setBounds (getWidth() - 82, 24, 60, 22);

    if (bottomComponent != nullptr)
        bottomComponent->setBounds (18, getHeight() - 18 - bottomHeight,
                                    getWidth() - 36, bottomHeight);

    if (knobs.isEmpty())
        return;

    const auto area = getKnobArea();
    const int rows = (knobs.size() + columns - 1) / columns;
    const float cellWidth = static_cast<float> (area.getWidth()) / static_cast<float> (columns);
    const float cellHeight = static_cast<float> (area.getHeight()) / static_cast<float> (juce::jmax (1, rows));
    const int componentWidth = juce::roundToInt (juce::jmin (cellWidth - 8.0f, columns == 2 ? 138.0f : 108.0f));
    const int componentHeight = juce::roundToInt (juce::jmin (cellHeight - 2.0f, 132.0f));

    for (int i = 0; i < knobs.size(); ++i)
    {
        const int column = i % columns;
        const int row = i / columns;
        const float cellX = static_cast<float> (area.getX()) + cellWidth * static_cast<float> (column);
        const float cellY = static_cast<float> (area.getY()) + cellHeight * static_cast<float> (row);
        const auto cell = juce::Rectangle<float> (cellX, cellY, cellWidth, cellHeight);

        knobs[i]->setBounds (juce::Rectangle<int> (componentWidth, componentHeight)
                                 .withCentre (cell.getCentre().roundToInt()));
    }
}

void EffectPanel::paint (juce::Graphics& g)
{
    drawCardBackground (g, getLocalBounds().toFloat().reduced (0.5f), accentColour, effectEnabled);

    auto header = getLocalBounds().reduced (18, 0);
    g.setFont (font (8.5f, FontWeight::semibold, 0.14f));
    g.setColour (accentColour.withAlpha (effectEnabled ? 0.92f : 0.42f));
    g.drawText (panelNumber, header.removeFromTop (26).withTrimmedTop (8),
                juce::Justification::centredLeft, false);

    if (! effectEnabled)
    {
        g.setFont (font (7.5f, FontWeight::semibold, 0.13f));
        g.setColour (textDim);
        g.drawText ("BYPASSED",
                    juce::Rectangle<int> (getWidth() - 144, 12, 58, 12),
                    juce::Justification::centredRight, false);
    }

    g.setFont (font (20.0f, FontWeight::black, 0.005f));
    g.setColour (text.withAlpha (effectEnabled ? 1.0f : 0.7f));
    g.drawText (panelTitle, header.removeFromTop (30).withTrimmedTop (8),
                juce::Justification::centredLeft, false);

    g.setFont (font (8.0f, FontWeight::semibold, 0.115f));
    g.setColour (textMuted.withAlpha (effectEnabled ? 0.9f : 0.5f));
    g.drawText (panelSubtitle, header.removeFromTop (18),
                juce::Justification::centredLeft, false);

    g.setColour (border);
    g.fillRect (juce::Rectangle<float> (18.0f, 84.0f, static_cast<float> (getWidth() - 36), 1.0f));

    if (bottomComponent != nullptr)
    {
        const float lineY = static_cast<float> (getHeight() - 18 - bottomHeight - 10);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.fillRect (juce::Rectangle<float> (18.0f, lineY, static_cast<float> (getWidth() - 36), 1.0f));
    }
}

} // namespace cerberus::ui
