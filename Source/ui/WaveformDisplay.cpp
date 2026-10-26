#include "WaveformDisplay.h"

#include "PluginProcessor.h"
#include "Theme.h"

#include <cmath>

namespace cerberus::ui
{

WaveformDisplay::WaveformDisplay (CerberusAudioProcessor& p)
    : processor (p)
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (30);
}

void WaveformDisplay::timerCallback()
{
    const auto size = static_cast<int> (CerberusAudioProcessor::waveformSize);
    const int write = static_cast<int> (processor.getWaveformWriteIndex());

    float peak = 0.0f;

    for (int i = 0; i < numColumns; ++i)
    {
        const int index = ((write - numColumns + i) % size + size) % size;
        const float value = processor.getWaveformSample (static_cast<uint32_t> (index));
        samples[static_cast<size_t> (i)] = value;
        peak = juce::jmax (peak, std::abs (value));
    }

    const float targetGain = peak > 1.0e-5f ? juce::jlimit (1.0f, 5.0f, 0.72f / peak) : 1.0f;
    displayGain += 0.14f * (targetGain - displayGain);

    repaint();
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    constexpr float corner = 6.0f;

    // 1. Recessed obsidian screen under the glass.
    g.setColour (obsidianScreen);
    g.fillRoundedRectangle (bounds, corner);

    const auto inner = bounds.reduced (2.0f);

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path screenClip;
        screenClip.addRoundedRectangle (inner, corner - 1.0f);
        g.reduceClipRegion (screenClip);

        // Small hardware readout caption inside the screen.
        g.setFont (font (6.0f, FontWeight::semibold, 0.18f));
        g.setColour (textMuted);
        g.drawText ("LIVE OUTPUT WAVEFORM", inner.getX() + 9.0f, inner.getY() + 4.0f,
                    200.0f, 9.0f, juce::Justification::centredLeft, false);

        g.setColour (crimsonAccent.withAlpha (0.65f));
        g.fillRect (inner.getX() + 9.0f, inner.getY() + 13.0f, 26.0f, 0.8f);

        g.setColour (textMuted);
        g.setFont (font (5.8f, FontWeight::semibold, 0.14f));
        g.drawText ("STEREO  /  POST FX  /  48 kHz", inner.getRight() - 180.0f, inner.getY() + 4.0f,
                    171.0f, 9.0f, juce::Justification::centredRight, false);

        const auto grid = inner.withTrimmedTop (15.0f).withTrimmedBottom (3.0f);
        const auto plotting = grid.withTrimmedLeft (50.0f).withTrimmedRight (12.0f);

        // 2. Subtle dB and time grid hairlines.
        constexpr int numHorizontalLines = 5;
        const char* dbLabels[numHorizontalLines] { "0 dB", "-6 dB", "-12 dB", "-18 dB", "-24 dB" };

        g.setFont (font (6.4f, FontWeight::regular, 0.02f));

        for (int i = 0; i < numHorizontalLines; ++i)
        {
            const float prop = static_cast<float> (i + 1) / static_cast<float> (numHorizontalLines + 1);
            const float y = grid.getY() + prop * grid.getHeight();

            g.setColour (i == 2 ? obsidianGrid.brighter (0.16f) : obsidianGrid);
            g.fillRect (grid.getX() + 38.0f, y, grid.getWidth() - 46.0f, 1.0f);

            g.setColour (i == 2 ? textSecondary.withAlpha (0.72f) : textMuted);
            g.drawText (dbLabels[i],
                        juce::Rectangle<float> (grid.getX() + 7.0f, y - 6.0f, 40.0f, 11.0f),
                        juce::Justification::centredLeft, false);
        }

        constexpr int numVerticalLines = 10;

        for (int i = 1; i < numVerticalLines; ++i)
        {
            const float prop = static_cast<float> (i) / static_cast<float> (numVerticalLines);
            const float x = plotting.getX() + prop * plotting.getWidth();
            g.setColour (obsidianGrid.withAlpha (i == 5 ? 1.0f : 0.65f));
            g.fillRect (x, grid.getY(), 1.0f, grid.getHeight());
        }

        // 3. Glowing crimson live waveform, three-pass LED trace.
        const float midY  = plotting.getCentreY();
        const float halfH = plotting.getHeight() * 0.46f;

        juce::Path trace;
        juce::Path fill;

        for (int i = 0; i < numColumns; ++i)
        {
            const float normX = static_cast<float> (i) / static_cast<float> (numColumns - 1);
            const float x = plotting.getX() + normX * plotting.getWidth();
            const float value = juce::jlimit (-1.0f, 1.0f,
                                              samples[static_cast<size_t> (i)] * displayGain);
            const float y = midY - value * halfH;

            if (i == 0)
            {
                trace.startNewSubPath (x, y);
                fill.startNewSubPath (x, midY);
                fill.lineTo (x, y);
            }
            else
            {
                trace.lineTo (x, y);
                fill.lineTo (x, y);
            }
        }

        fill.lineTo (plotting.getRight(), midY);
        fill.closeSubPath();

        juce::ColourGradient envelope (crimsonGlow.withAlpha (0.04f),
                                       plotting.getX(), midY - halfH,
                                       crimsonAccent.withAlpha (0.20f),
                                       plotting.getX(), midY,
                                       false);
        envelope.addColour (0.5, crimsonGlow.withAlpha (0.10f));
        g.setGradientFill (envelope);
        g.fillPath (fill);

        g.setColour (crimsonGlow.withAlpha (0.26f));
        g.strokePath (trace, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (crimsonAccent);
        g.strokePath (trace, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        g.setColour (crimsonBright);
        g.strokePath (trace, juce::PathStrokeType (0.9f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // 4. Glossy upper-left glass reflection over the embedded display.
        juce::Path sheen;
        sheen.startNewSubPath (inner.getX(), inner.getY());
        sheen.lineTo (inner.getX() + inner.getWidth() * 0.42f, inner.getY());
        sheen.lineTo (inner.getX() + inner.getWidth() * 0.18f, inner.getBottom());
        sheen.lineTo (inner.getX(), inner.getBottom());
        sheen.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.028f));
        g.fillPath (sheen);
    }

    // 5. Recessed glass bezel and subtle crimson inner refraction.
    g.setColour (crimsonAccent.withAlpha (0.42f));
    g.drawRoundedRectangle (bounds, corner, 1.1f);
    g.setColour (glassRimHighlight.withAlpha (0.16f));
    g.drawRoundedRectangle (bounds.reduced (1.0f), corner - 1.0f, 0.8f);
}

} // namespace cerberus::ui
