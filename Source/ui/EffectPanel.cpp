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
                          int knobColumns,
                          juce::String buttonLabel)
    : apvts (state),
      panelNumber (std::move (number)),
      panelTitle (std::move (title)),
      panelSubtitle (std::move (subtitle)),
      powerLabel (std::move (buttonLabel)),
      accentColour (accent),
      columns (juce::jmax (1, knobColumns))
{
    powerSwitch = std::make_unique<PowerSwitch> (apvts, powerParameterID, powerLabel,
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
    auto area = getLocalBounds().reduced (12, 0);
    const int bottomTop = bottomComponent != nullptr ? getHeight() - 14 - bottomHeight : getHeight() - 12;
    area.setTop (84);
    area.setBottom (juce::jmax (92, bottomTop - 8));
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
        powerSwitch->setBounds (getWidth() - 114, 18, 100, 28);

    if (bottomComponent != nullptr)
        bottomComponent->setBounds (14, getHeight() - 14 - bottomHeight,
                                    getWidth() - 28, bottomHeight);

    if (knobs.isEmpty())
        return;

    const auto area = getKnobArea();
    const int rows = (knobs.size() + columns - 1) / columns;
    const float cellWidth = static_cast<float> (area.getWidth()) / static_cast<float> (columns);
    const float cellHeight = static_cast<float> (area.getHeight()) / static_cast<float> (juce::jmax (1, rows));
    const int componentWidth = juce::roundToInt (juce::jmin (cellWidth - 8.0f, columns == 2 ? 150.0f : 104.0f));
    const int componentHeight = juce::roundToInt (juce::jmin (cellHeight - 2.0f, 122.0f));

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
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    constexpr float corner = 9.0f;

    // Recessed glass module seat.
    g.setColour (obsidianWell.withAlpha (0.58f));
    g.fillRoundedRectangle (bounds, corner);

    juce::ColourGradient surface (gunmetalBase.withAlpha (0.18f), bounds.getTopLeft(),
                                  juce::Colours::transparentBlack, bounds.getBottomLeft(), false);
    g.setGradientFill (surface);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillRoundedRectangle (bounds.withHeight (4.0f), corner);

    g.setColour (glassRimHighlight.withAlpha (0.12f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, 0.8f);

    g.setColour (crimsonAccent.withAlpha (effectEnabled ? 0.18f : 0.07f));
    g.drawRoundedRectangle (bounds.reduced (1.5f), corner - 0.8f, 0.7f);

    // Calibration hairlines at the top corners.
    g.setColour (crimsonAccent.withAlpha (effectEnabled ? 0.72f : 0.22f));
    g.fillRect (bounds.getX() + 14.0f, bounds.getY() + 7.0f, 18.0f, 1.2f);
    g.fillRect (bounds.getRight() - 32.0f, bounds.getY() + 7.0f, 18.0f, 1.2f);

    // Engraved stage header.
    g.setFont (font (7.4f, FontWeight::semibold, 0.20f));
    g.setColour (accentColour.withAlpha (effectEnabled ? 0.95f : 0.42f));
    g.drawText (panelNumber, 16, 14, 30, 11, juce::Justification::centredLeft, false);

    g.setFont (font (16.5f, FontWeight::black, 0.015f));
    g.setColour (textPrimary.withAlpha (effectEnabled ? 1.0f : 0.56f));
    g.drawText (panelTitle, 16, 26, getWidth() - 140, 24,
                juce::Justification::centredLeft, false);

    g.setFont (font (6.5f, FontWeight::semibold, 0.14f));
    g.setColour (textMuted.withAlpha (effectEnabled ? 0.92f : 0.50f));
    g.drawText (panelSubtitle, 16, 51, getWidth() - 130, 11,
                juce::Justification::centredLeft, false);

    // Hairline separation below the header and above the meter row.
    juce::ColourGradient headerLine (crimsonSubtle.withAlpha (effectEnabled ? 0.90f : 0.35f),
                                     16.0f, 78.0f,
                                     juce::Colours::transparentBlack, static_cast<float> (getWidth() - 16), 78.0f,
                                     false);
    g.setGradientFill (headerLine);
    g.fillRect (juce::Rectangle<float> (16.0f, 78.0f, static_cast<float> (getWidth() - 32), 1.0f));

    if (bottomComponent != nullptr)
    {
        const float lineY = static_cast<float> (getHeight() - 14 - bottomHeight - 8);
        juce::ColourGradient footerLine (crimsonSubtle.withAlpha (effectEnabled ? 0.58f : 0.22f),
                                         16.0f, lineY,
                                         juce::Colours::transparentBlack, static_cast<float> (getWidth() - 16), lineY,
                                         false);
        g.setGradientFill (footerLine);
        g.fillRect (juce::Rectangle<float> (16.0f, lineY, static_cast<float> (getWidth() - 32), 1.0f));
    }

    if (! effectEnabled)
    {
        g.setFont (font (6.2f, FontWeight::semibold, 0.16f));
        g.setColour (textMuted);
        g.drawText ("BYPASSED", 16, 65, 60, 9, juce::Justification::centredLeft, false);
    }
}

} // namespace cerberus::ui
