#include "Controls.h"

#include <algorithm>
#include <cmath>

namespace cerberus::ui
{

//==============================================================================
ParamKnob::ParamKnob (juce::AudioProcessorValueTreeState& state,
                      const juce::String& parameterID,
                      const juce::String& displayName,
                      Formatter formatter,
                      juce::Colour accent)
    : valueFormatter (std::move (formatter)),
      accentColour (accent),
      parameter (state.getParameter (parameterID))
{
    jassert (parameter != nullptr);

    nameLabel.setText (displayName, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (font (9.0f, FontWeight::semibold, 0.10f));
    nameLabel.setColour (juce::Label::textColourId, textMuted);
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setFont (font (11.0f, FontWeight::semibold, 0.01f));
    valueLabel.setColour (juce::Label::textColourId, accentColour.withAlpha (0.94f));
    valueLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (valueLabel);

    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.18f,
                                juce::MathConstants<float>::pi * 2.82f,
                                true);
    slider.setMouseDragSensitivity (190);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::thumbColourId, accentColour);
    slider.setTooltip (displayName);
    slider.setName (displayName);
    addAndMakeVisible (slider);

    slider.textFromValueFunction = [this] (double value)
    {
        return valueFormatter (static_cast<float> (value));
    };

    slider.onValueChange = [this] { updateValueLabel(); };

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, parameterID, slider);
    updateValueLabel();
}

void ParamKnob::setAccent (juce::Colour newAccent)
{
    accentColour = newAccent;
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::thumbColourId, accentColour);
    valueLabel.setColour (juce::Label::textColourId, accentColour.withAlpha (0.94f));
}

void ParamKnob::resized()
{
    auto area = getLocalBounds();
    area.reduce (1, 0);

    constexpr int nameHeight = 13;
    constexpr int valueHeight = 17;
    const int availableForKnob = juce::jmax (10, area.getHeight() - nameHeight - valueHeight);
    const int diameter = juce::jmax (10, juce::jmin (area.getWidth(), availableForKnob));

    auto knobArea = area.removeFromTop (availableForKnob);
    slider.setBounds (knobArea.withSizeKeepingCentre (diameter, diameter));

    nameLabel.setBounds (area.removeFromTop (nameHeight));
    valueLabel.setBounds (area.removeFromTop (valueHeight));
}

void ParamKnob::updateValueLabel()
{
    valueLabel.setText (valueFormatter (static_cast<float> (slider.getValue())),
                        juce::dontSendNotification);
}

//==============================================================================
PowerSwitch::PowerSwitch (juce::AudioProcessorValueTreeState& state,
                          const juce::String& parameterID,
                          juce::Colour accent,
                          std::function<void (bool)> onChanged)
    : juce::Button ("Power"),
      parameter (state.getParameter (parameterID)),
      changeCallback (std::move (onChanged)),
      accentColour (accent)
{
    jassert (parameter != nullptr);

    setTooltip ("Enable or bypass this module");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setWantsKeyboardFocus (true);

    attachment = std::make_unique<juce::ParameterAttachment> (*parameter, [this] (float value)
    {
        setOn (value > 0.5f);
    });

    attachment->sendInitialUpdate();
}

void PowerSwitch::clicked()
{
    setOn (! on);
    attachment->setValueAsCompleteGesture (on ? 1.0f : 0.0f);
}

void PowerSwitch::setOn (bool shouldBeOn)
{
    if (on == shouldBeOn)
        return;

    on = shouldBeOn;
    repaint();

    if (changeCallback != nullptr)
        changeCallback (on);
}

void PowerSwitch::paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    const auto bounds = getLocalBounds().toFloat();
    const float trackHeight = 17.0f;
    auto track = juce::Rectangle<float> (0.0f, (bounds.getHeight() - trackHeight) * 0.5f,
                                         31.0f, trackHeight);

    if (isOn())
    {
        g.setColour (accentColour.withAlpha (isMouseOverButton ? 0.30f : 0.20f));
        g.fillRoundedRectangle (track.expanded (2.5f), 10.0f);
    }

    g.setColour (isOn() ? accentColour.withAlpha (isMouseOverButton ? 0.32f : 0.22f)
                        : panelDeep);
    g.fillRoundedRectangle (track, 8.5f);

    g.setColour (isOn() ? accentColour.withAlpha (0.75f) : border.brighter (0.35f));
    g.drawRoundedRectangle (track.reduced (0.5f), 8.0f, 1.0f);

    const float thumbX = isOn() ? track.getRight() - 9.0f : track.getX() + 9.0f;
    g.setColour (isOn() ? accentColour : textDim);
    g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre ({ thumbX, track.getCentreY() }));

    if (isOn())
    {
        g.setColour (juce::Colours::white.withAlpha (0.42f));
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (
            { thumbX - 1.5f, track.getCentreY() - 2.0f }));
    }

    g.setFont (font (8.0f, FontWeight::semibold, 0.12f));
    g.setColour (isOn() ? accentColour : textDim);
    g.drawText (isOn() ? "ON" : "OFF",
                juce::Rectangle<float> (36.0f, 0.0f, bounds.getWidth() - 36.0f, bounds.getHeight()),
                juce::Justification::centredLeft, false);

    if (isButtonDown)
    {
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (bounds, 4.0f);
    }
}

//==============================================================================
ModeBar::ModeBar (juce::AudioProcessorValueTreeState& state,
                  const juce::String& parameterID,
                  juce::Colour accent)
    : parameter (state.getParameter (parameterID)),
      accentColour (accent)
{
    jassert (parameter != nullptr);
    choices = { "Warm", "Crunch", "Fuzz" };
    setMouseCursor (juce::MouseCursor::PointingHandCursor);

    attachment = std::make_unique<juce::ParameterAttachment> (*parameter, [this] (float value)
    {
        setSelected (juce::roundToInt (value));
    });

    attachment->sendInitialUpdate();
}

juce::Rectangle<float> ModeBar::segmentBounds (int index) const
{
    const auto area = getLocalBounds().toFloat();
    const float gap = 5.0f;
    const float width = (area.getWidth() - gap * 2.0f) / 3.0f;
    const float x = static_cast<float> (index) * (width + gap);
    return { area.getX() + x, area.getY() + 12.0f, width, juce::jmax (16.0f, area.getHeight() - 12.0f) };
}

int ModeBar::indexAt (juce::Point<float> position) const
{
    for (int i = 0; i < 3; ++i)
        if (segmentBounds (i).contains (position))
            return i;

    return -1;
}

void ModeBar::paint (juce::Graphics& g)
{
    g.setFont (font (8.0f, FontWeight::semibold, 0.12f));
    g.setColour (textMuted);
    g.drawText ("CHARACTER", getLocalBounds().removeFromTop (11),
                juce::Justification::centredLeft, false);

    for (int i = 0; i < choices.size(); ++i)
    {
        const auto segment = segmentBounds (i);
        const bool active = i == selected;
        const bool isHovered = i == hovered;

        g.setColour (active ? accentColour.withAlpha (0.18f)
                            : (isHovered ? juce::Colours::white.withAlpha (0.055f) : panelDeep.withAlpha (0.8f)));
        g.fillRoundedRectangle (segment, 4.5f);

        g.setColour (active ? accentColour.withAlpha (0.75f)
                            : (isHovered ? border.brighter (0.8f) : border));
        g.drawRoundedRectangle (segment.reduced (0.5f), 4.0f, 1.0f);

        if (active)
        {
            g.setColour (accentColour);
            g.fillRoundedRectangle (segment.getX() + 8.0f, segment.getBottom() - 2.0f,
                                    segment.getWidth() - 16.0f, 1.5f, 0.75f);
        }

        g.setFont (font (9.5f, FontWeight::semibold, 0.035f));
        g.setColour (active ? text : textMuted);
        g.drawText (choices[i], segment, juce::Justification::centred, false);
    }
}

void ModeBar::mouseDown (const juce::MouseEvent& event)
{
    const int index = indexAt (event.position);

    if (index >= 0)
        attachment->setValueAsCompleteGesture (static_cast<float> (index));
}

void ModeBar::mouseMove (const juce::MouseEvent& event)
{
    const int newHovered = indexAt (event.position);

    if (newHovered != hovered)
    {
        hovered = newHovered;
        repaint();
    }
}

void ModeBar::mouseExit (const juce::MouseEvent&)
{
    if (hovered >= 0)
    {
        hovered = -1;
        repaint();
    }
}

void ModeBar::setSelected (int newIndex)
{
    newIndex = juce::jlimit (0, choices.size() - 1, newIndex);

    if (newIndex != selected)
    {
        selected = newIndex;
        repaint();
    }
}

//==============================================================================
GainReductionMeter::GainReductionMeter (juce::Colour accent)
    : accentColour (accent)
{
    setInterceptsMouseClicks (false, false);
}

void GainReductionMeter::setReductionDb (float newReductionDb)
{
    const float target = juce::jlimit (0.0f, 24.0f, newReductionDb);

    if (target > displayedDb)
        displayedDb = target;
    else
        displayedDb += 0.22f * (target - displayedDb);

    if (displayedDb < 0.03f)
        displayedDb = 0.0f;

    repaint();
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    g.setFont (font (8.0f, FontWeight::semibold, 0.10f));
    g.setColour (textMuted);
    g.drawText ("GAIN REDUCTION", area.removeFromTop (12.0f),
                juce::Justification::centredLeft, false);
    g.setColour (accentColour);
    g.setFont (font (9.0f, FontWeight::semibold, 0.02f));
    g.drawText (displayedDb > 0.05f ? "-" + juce::String (displayedDb, 1) + " dB" : "0.0 dB",
                getLocalBounds().toFloat().removeFromTop (12.0f),
                juce::Justification::centredRight, false);

    auto bar = getLocalBounds().toFloat().withTrimmedTop (16.0f).withHeight (8.0f);

    g.setColour (panelDeep);
    g.fillRoundedRectangle (bar, 4.0f);

    const float proportion = juce::jlimit (0.0f, 1.0f, displayedDb / 24.0f);

    if (proportion > 0.002f)
    {
        auto fill = bar.withWidth (juce::jmax (3.0f, bar.getWidth() * proportion));
        juce::ColourGradient grad (accentColour, bar.getX(), bar.getY(),
                                   distortion, bar.getX() + bar.getWidth() * 0.85f, bar.getY(), false);
        grad.addColour (0.55, amber);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 4.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (int i = 1; i < 6; ++i)
    {
        const float x = bar.getX() + bar.getWidth() * static_cast<float> (i) / 6.0f;
        g.drawVerticalLine (juce::roundToInt (x), bar.getY(), bar.getBottom());
    }

    g.setFont (font (7.0f, FontWeight::regular));
    g.setColour (textDim);
    g.drawText ("0", getLocalBounds().toFloat().removeFromBottom (9.0f),
                juce::Justification::centredLeft, false);
    g.drawText ("24", getLocalBounds().toFloat().removeFromBottom (9.0f),
                juce::Justification::centredRight, false);
}

//==============================================================================
SweepMeter::SweepMeter (juce::Colour accent)
    : accentColour (accent)
{
    setInterceptsMouseClicks (false, false);
}

void SweepMeter::setCutoffHz (float newCutoffHz)
{
    const float target = juce::jlimit (80.0f, 8000.0f, newCutoffHz);
    displayedHz += 0.32f * (target - displayedHz);
    repaint();
}

void SweepMeter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    g.setFont (font (8.0f, FontWeight::semibold, 0.10f));
    g.setColour (textMuted);
    g.drawText ("ENVELOPE SWEEP", area.removeFromTop (12.0f),
                juce::Justification::centredLeft, false);
    g.setColour (accentColour);
    g.setFont (font (9.0f, FontWeight::semibold, 0.02f));
    g.drawText (formatHz (displayedHz), getLocalBounds().toFloat().removeFromTop (12.0f),
                juce::Justification::centredRight, false);

    auto bar = getLocalBounds().toFloat().withTrimmedTop (16.0f).withHeight (8.0f);
    g.setColour (panelDeep);
    g.fillRoundedRectangle (bar, 4.0f);

    const float normalised = juce::jlimit (0.0f, 1.0f,
        std::log (displayedHz / 80.0f) / std::log (8000.0f / 80.0f));

    juce::ColourGradient grad (accentColour.withAlpha (0.55f), bar.getX(), bar.getY(),
                               accentColour, bar.getRight(), bar.getY(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bar.withWidth (juce::jmax (4.0f, bar.getWidth() * normalised)), 4.0f);

    const float markerX = bar.getX() + bar.getWidth() * normalised;
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.fillRoundedRectangle (markerX - 1.0f, bar.getY() - 3.0f, 2.0f, 14.0f, 1.0f);

    g.setFont (font (7.0f, FontWeight::regular));
    g.setColour (textDim);
    g.drawText ("80 Hz", getLocalBounds().toFloat().removeFromBottom (9.0f),
                juce::Justification::centredLeft, false);
    g.drawText ("8 kHz", getLocalBounds().toFloat().removeFromBottom (9.0f),
                juce::Justification::centredRight, false);
}

//==============================================================================
StereoMeter::StereoMeter()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Output level — click to reset clip indicator");
}

float StereoMeter::levelToNormalised (float level)
{
    const float db = juce::Decibels::gainToDecibels (std::max (level, 1.0e-5f), -72.0f);
    return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f);
}

void StereoMeter::setLevels (float left, float right)
{
    const float targetL = levelToNormalised (left);
    const float targetR = levelToNormalised (right);

    displayLeft = targetL > displayLeft ? targetL : juce::jmax (targetL, displayLeft - 0.022f);
    displayRight = targetR > displayRight ? targetR : juce::jmax (targetR, displayRight - 0.022f);

    if (displayLeft >= peakLeft || displayRight >= peakRight)
    {
        peakLeft = std::max (peakLeft, displayLeft);
        peakRight = std::max (peakRight, displayRight);
        peakHoldTicks = 32;
    }
    else if (--peakHoldTicks <= 0)
    {
        peakLeft = juce::jmax (displayLeft, peakLeft - 0.008f);
        peakRight = juce::jmax (displayRight, peakRight - 0.008f);
    }

    if (left > 1.0f || right > 1.0f)
        clipTicks = 45;
    else if (clipTicks > 0)
        --clipTicks;

    repaint();
}

void StereoMeter::mouseDown (const juce::MouseEvent&)
{
    clipTicks = 0;
    repaint();
}

void StereoMeter::paintBar (juce::Graphics& g, juce::Rectangle<float> bounds, float level, float peak) const
{
    g.setColour (panelDeep);
    g.fillRoundedRectangle (bounds, 3.5f);

    if (level > 0.002f)
    {
        auto fill = bounds.withWidth (juce::jmax (3.0f, bounds.getWidth() * level));
        juce::ColourGradient grad (meterGreen, bounds.getX(), bounds.getY(),
                                   meterRed, bounds.getRight(), bounds.getY(), false);
        grad.addColour (0.66, amber);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 3.5f);
    }

    const float peakX = bounds.getX() + bounds.getWidth() * peak;
    if (peak > 0.01f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.72f));
        g.fillRect (peakX - 1.0f, bounds.getY() - 1.0f, 1.5f, bounds.getHeight() + 2.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    for (int tick = 1; tick < 10; ++tick)
    {
        const float x = bounds.getX() + bounds.getWidth() * static_cast<float> (tick) / 10.0f;
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());
    }
}

void StereoMeter::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    g.setFont (font (8.0f, FontWeight::semibold, 0.11f));
    g.setColour (textMuted);
    g.drawText ("OUTPUT", area.removeFromTop (12.0f).removeFromLeft (100.0f),
                juce::Justification::centredLeft, false);

    const bool clipping = clipTicks > 0;
    g.setFont (font (8.0f, FontWeight::semibold, 0.08f));
    g.setColour (clipping ? meterRed : textDim);
    g.drawText ("CLIP", getLocalBounds().toFloat().removeFromTop (12.0f),
                juce::Justification::centredRight, false);

    auto barArea = getLocalBounds().toFloat().withTrimmedTop (17.0f);
    auto leftBar = barArea.removeFromTop (9.0f);
    barArea.removeFromTop (5.0f);
    auto rightBar = barArea.removeFromTop (9.0f);

    g.setFont (font (7.5f, FontWeight::semibold));
    g.setColour (textDim);
    g.drawText ("L", leftBar.removeFromLeft (12.0f), juce::Justification::centredLeft, false);
    g.drawText ("R", rightBar.removeFromLeft (12.0f), juce::Justification::centredLeft, false);

    paintBar (g, leftBar, displayLeft, peakLeft);
    paintBar (g, rightBar, displayRight, peakRight);

    g.setFont (font (6.5f, FontWeight::regular));
    g.setColour (textDim);
    const float barX = getLocalBounds().toFloat().getX() + 12.0f;
    const float barW = getLocalBounds().toFloat().getWidth() - 12.0f;
    const float labelY = static_cast<float> (getHeight()) - 9.0f;
    const std::pair<float, const char*> labels[] { { 0.0f, "-60" }, { 0.43f, "-24" },
                                                   { 0.76f, "-6" }, { 1.0f, "0" } };

    for (const auto& label : labels)
    {
        const float x = barX + barW * label.first;
        g.drawText (label.second,
                    juce::Rectangle<float> (x - 10.0f, labelY, 20.0f, 8.0f),
                    juce::Justification::centred, false);
    }
}

} // namespace cerberus::ui
