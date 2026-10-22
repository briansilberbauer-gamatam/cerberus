#include "CerberusLookAndFeel.h"

#include <cmath>

namespace cerberus::ui
{

CerberusLookAndFeel::CerberusLookAndFeel()
{
    setColour (juce::Label::textColourId, text);
    setColour (juce::Label::textWhenEditingColourId, text);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::ComboBox::backgroundColourId, panelDeep);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, border.brighter (0.5f));
    setColour (juce::ComboBox::arrowColourId, textMuted);
    setColour (juce::ComboBox::focusedOutlineColourId, wah.withAlpha (0.55f));

    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::headerTextColourId, textMuted);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, wah.withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId, text);

    setColour (juce::TextButton::buttonColourId, panelLift);
    setColour (juce::TextButton::buttonOnColourId, wah);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, background);

    setColour (juce::TooltipWindow::backgroundColourId, panelLift);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::TooltipWindow::outlineColourId, border.brighter (0.6f));

    setColour (juce::Slider::rotarySliderOutlineColourId, panelDeep);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void CerberusLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPosProportional, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                static_cast<float> (width), static_cast<float> (height))
                            .reduced (2.0f);

    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    if (radius < 4.0f)
        return;

    const auto centre = bounds.getCentre();
    const float arcRadius = radius - 2.0f;
    const float knobRadius = juce::jmax (5.0f, radius * 0.605f);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const float angle = rotaryStartAngle + juce::jlimit (0.0f, 1.0f, sliderPosProportional)
                                           * (rotaryEndAngle - rotaryStartAngle);

    for (int i = 0; i <= 24; ++i)
    {
        const float proportion = static_cast<float> (i) / 24.0f;
        const float tickAngle = rotaryStartAngle + proportion * (rotaryEndAngle - rotaryStartAngle);
        const bool active = proportion <= sliderPosProportional + 0.001f;
        const float inner = arcRadius - (active ? 4.0f : 2.5f);
        const float outer = arcRadius + (active ? 4.2f : 3.0f);
        const auto p1 = centre.getPointOnCircumference (inner, tickAngle);
        const auto p2 = centre.getPointOnCircumference (outer, tickAngle);
        g.setColour (active ? accent.withAlpha (0.42f) : juce::Colours::white.withAlpha (0.075f));
        g.drawLine ({ p1, p2 }, active ? 1.2f : 0.8f);
    }

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.strokePath (track, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (sliderPosProportional > 0.001f)
    {
        juce::Path activeArc;
        activeArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 rotaryStartAngle, angle, true);

        g.setColour (accent.withAlpha (0.14f));
        g.strokePath (activeArc, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (activeArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    const juce::Rectangle<float> knobBounds (centre.x - knobRadius, centre.y - knobRadius,
                                             knobRadius * 2.0f, knobRadius * 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.42f));
    g.fillEllipse (knobBounds.translated (0.0f, 2.5f));

    juce::ColourGradient body (panelLift.brighter (0.18f), knobBounds.getTopLeft(),
                               panelDeep.darker (0.25f), knobBounds.getBottomLeft(), false);
    body.addColour (0.52, panel);
    g.setGradientFill (body);
    g.fillEllipse (knobBounds);

    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.drawEllipse (knobBounds, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.drawEllipse (knobBounds.reduced (5.0f), 1.0f);

    const auto pointerStart = centre.getPointOnCircumference (knobRadius * 0.28f, angle);
    const auto pointerEnd = centre.getPointOnCircumference (knobRadius * 0.79f, angle);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine ({ pointerStart.translated (0.8f, 1.2f), pointerEnd.translated (0.8f, 1.2f) }, 3.0f);
    g.setColour (accent);
    g.drawLine ({ pointerStart, pointerEnd }, 2.0f);
    g.fillEllipse (juce::Rectangle<float> (4.2f, 4.2f).withCentre (pointerEnd));

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (
        centre.getPointOnCircumference (knobRadius * 0.36f, angle + juce::MathConstants<float>::halfPi)));
}

void CerberusLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                        int, int, int, int, juce::ComboBox& box)
{
    const juce::Rectangle<float> bounds (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));

    g.setColour (panelDeep);
    g.fillRoundedRectangle (bounds.reduced (0.5f), 6.0f);
    g.setColour (box.hasKeyboardFocus (true) ? wah.withAlpha (0.55f) : border.brighter (0.55f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    const float arrowX = bounds.getRight() - 15.0f;
    const float arrowY = bounds.getCentreY();
    juce::Path chevron;
    chevron.startNewSubPath (arrowX - 4.0f, arrowY - 2.0f);
    chevron.lineTo (arrowX, arrowY + 2.5f);
    chevron.lineTo (arrowX + 4.0f, arrowY - 2.0f);
    g.setColour (box.isMouseOver (true) ? text : textMuted);
    g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

void CerberusLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    auto colour = backgroundColour;

    if (shouldDrawButtonAsDown)
        colour = colour.brighter (0.12f);
    else if (shouldDrawButtonAsHighlighted)
        colour = colour.brighter (0.07f);

    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (shouldDrawButtonAsHighlighted ? wah.withAlpha (0.32f) : border.brighter (0.35f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void CerberusLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                          bool, bool)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                             : juce::TextButton::textColourOffId)
                    .withAlpha (button.isEnabled() ? 1.0f : 0.45f));

    g.drawText (button.getButtonText(), button.getLocalBounds().reduced (2, 0),
                juce::Justification::centred, false);
}

void CerberusLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 7.0f);
    g.setColour (border.brighter (0.45f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 7.0f, 1.0f);
}

void CerberusLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (11, 1, box.getWidth() - 38, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

juce::Font CerberusLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return font (12.0f, FontWeight::semibold, 0.015f);
}

juce::Font CerberusLookAndFeel::getPopupMenuFont()
{
    return font (12.5f, FontWeight::regular, 0.005f);
}

juce::Font CerberusLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return font (juce::jmin (18.0f, static_cast<float> (buttonHeight) * 0.56f), FontWeight::semibold);
}

} // namespace cerberus::ui
