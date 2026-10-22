#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace cerberus::ui
{

inline const juce::Colour background { 0xff080a0d };
inline const juce::Colour backgroundLift { 0xff10151b };
inline const juce::Colour panel { 0xff14191f };
inline const juce::Colour panelLift { 0xff1b222a };
inline const juce::Colour panelDeep { 0xff0d1116 };
inline const juce::Colour border { 0x1fffffff };
inline const juce::Colour gridLine { 0x08ffffff };
inline const juce::Colour text { 0xffeef2f4 };
inline const juce::Colour textMuted { 0xff8b969f };
inline const juce::Colour textDim { 0xff525d67 };

inline const juce::Colour wah { 0xffffb65c };
inline const juce::Colour compressor { 0xff55ddc8 };
inline const juce::Colour distortion { 0xffff7166 };
inline const juce::Colour amber { 0xffffc56e };
inline const juce::Colour meterGreen { 0xff5de2b4 };
inline const juce::Colour meterRed { 0xffff5f5f };

enum class FontWeight
{
    regular,
    semibold,
    black
};

juce::Font font (float height, FontWeight weight = FontWeight::regular, float tracking = 0.0f);

juce::String formatDb (float value);
juce::String formatPercent (float value);
juce::String formatTimeMs (float value);
juce::String formatHz (float value);
juce::String formatOctaves (float value);
juce::String formatRatio (float value);
juce::String formatTone (float value);

void drawCardBackground (juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         juce::Colour accent,
                         bool enabled,
                         float cornerSize = 14.0f);

void drawAccentDivider (juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        juce::Colour accent);

} // namespace cerberus::ui
