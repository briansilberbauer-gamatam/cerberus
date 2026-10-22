#include "Theme.h"

#include <BinaryData.h>

#include <cmath>

namespace cerberus::ui
{
namespace
{
juce::Typeface::Ptr makeTypeface (const void* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
}

const juce::Typeface::Ptr& regularTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoRegular_ttf, BinaryData::LatoRegular_ttfSize);
    return typeface;
}

const juce::Typeface::Ptr& semiboldTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoSemibold_ttf, BinaryData::LatoSemibold_ttfSize);
    return typeface;
}

const juce::Typeface::Ptr& blackTypeface()
{
    static const auto typeface = makeTypeface (BinaryData::LatoBlack_ttf, BinaryData::LatoBlack_ttfSize);
    return typeface;
}
} // namespace

juce::Font font (float height, FontWeight weight, float tracking)
{
    const auto& typeface = weight == FontWeight::black ? blackTypeface()
                         : weight == FontWeight::semibold ? semiboldTypeface()
                                                          : regularTypeface();

    juce::Font result { juce::FontOptions (typeface).withHeight (height) };

    if (! juce::approximatelyEqual (tracking, 0.0f))
        result = result.withExtraKerningFactor (tracking);

    return result;
}

juce::String formatDb (float value)
{
    if (std::abs (value) < 0.05f)
        return "0.0 dB";

    return (value > 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
}

juce::String formatPercent (float value)
{
    return juce::String (juce::roundToInt (juce::jlimit (0.0f, 1.0f, value) * 100.0f)) + "%";
}

juce::String formatTimeMs (float value)
{
    if (value < 10.0f)
        return juce::String (value, 1) + " ms";

    return juce::String (juce::roundToInt (value)) + " ms";
}

juce::String formatHz (float value)
{
    if (value < 1000.0f)
        return juce::String (juce::roundToInt (value)) + " Hz";

    return juce::String (value / 1000.0f, value < 10000.0f ? 2 : 1) + " kHz";
}

juce::String formatOctaves (float value)
{
    return juce::String (value, 1) + " oct";
}

juce::String formatRatio (float value)
{
    return juce::String (value, 1) + ":1";
}

juce::String formatTone (float value)
{
    return juce::String (value * 10.0f, 1);
}

void drawCardBackground (juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         juce::Colour accent,
                         bool enabled,
                         float cornerSize)
{
    const auto radius = cornerSize;
    const auto shadow = bounds.translated (0.0f, 4.0f).reduced (1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (shadow, radius);

    juce::ColourGradient fill (panelLift, bounds.getTopLeft(),
                               panelDeep, bounds.getBottomLeft(), false);
    fill.addColour (0.55, panel);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, radius);

    if (enabled)
    {
        juce::ColourGradient glow (accent.withAlpha (0.085f),
                                   bounds.getX() + bounds.getWidth() * 0.16f,
                                   bounds.getY(),
                                   accent.withAlpha (0.0f),
                                   bounds.getX() + bounds.getWidth() * 0.58f,
                                   bounds.getY() + bounds.getHeight() * 0.38f,
                                   true);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (bounds, radius);
    }

    g.setColour (accent.withAlpha (enabled ? 0.42f : 0.12f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);

    juce::Path topEdge;
    topEdge.addRoundedRectangle (bounds.getX() + 1.0f,
                                 bounds.getY() + 1.0f,
                                 bounds.getWidth() - 2.0f,
                                 bounds.getHeight() - 2.0f,
                                 radius, radius, true, true, false, false);

    g.setColour (accent.withAlpha (enabled ? 0.9f : 0.25f));
    g.strokePath (topEdge, juce::PathStrokeType (1.5f));
}

void drawAccentDivider (juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        juce::Colour accent)
{
    juce::ColourGradient line (accent.withAlpha (0.45f), bounds.getX(), bounds.getY(),
                               accent.withAlpha (0.02f), bounds.getRight(), bounds.getY(),
                               false);
    g.setGradientFill (line);
    g.fillRect (bounds.withHeight (1.0f));
}

} // namespace cerberus::ui
