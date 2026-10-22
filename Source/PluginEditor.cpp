#include "PluginEditor.h"

#include "Parameters.h"
#include "ui/EffectPanel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

using namespace cerberus::ui;

namespace
{
constexpr int designWidth = 1180;
constexpr int designHeight = 720;

const juce::Colour globalAccent { 0xffa9bac8 };

juce::String resonanceText (float value)
{
    return juce::String (value, 1);
}
} // namespace

class CerberusEditorContent final : public juce::Component,
                                    private juce::Timer
{
public:
    explicit CerberusEditorContent (CerberusAudioProcessor& p)
        : processor (p),
          apvts (p.getValueTreeState())
    {
        setOpaque (true);

        wahPanel = std::make_unique<EffectPanel> (apvts, "01", "AUTO-WAH",
                                                  "ENVELOPE-CONTROLLED FILTER",
                                                  wah, cerberus::params::wahOn, 3);
        addAndMakeVisible (*wahPanel);

        compPanel = std::make_unique<EffectPanel> (apvts, "02", "COMPRESSOR",
                                                   "FEED-FORWARD DYNAMICS",
                                                   compressor, cerberus::params::compOn, 3);
        addAndMakeVisible (*compPanel);

        distPanel = std::make_unique<EffectPanel> (apvts, "03", "DISTORTION",
                                                   "4x OVERSAMPLED SATURATION",
                                                   distortion, cerberus::params::distOn, 2);
        addAndMakeVisible (*distPanel);

        wahPanel->addKnob ("SENSITIVITY", cerberus::params::wahSensitivity, formatPercent);
        wahPanel->addKnob ("START", cerberus::params::wahStart, formatHz);
        wahPanel->addKnob ("RANGE", cerberus::params::wahRange, formatOctaves);
        wahPanel->addKnob ("RESONANCE", cerberus::params::wahResonance, resonanceText);
        wahPanel->addKnob ("ATTACK", cerberus::params::wahAttack, formatTimeMs);
        wahPanel->addKnob ("RELEASE", cerberus::params::wahRelease, formatTimeMs);

        compPanel->addKnob ("THRESHOLD", cerberus::params::compThreshold, formatDb);
        compPanel->addKnob ("RATIO", cerberus::params::compRatio, formatRatio);
        compPanel->addKnob ("ATTACK", cerberus::params::compAttack, formatTimeMs);
        compPanel->addKnob ("RELEASE", cerberus::params::compRelease, formatTimeMs);
        compPanel->addKnob ("MAKE-UP", cerberus::params::compMakeup, formatDb);
        compPanel->addKnob ("MIX", cerberus::params::compMix, formatPercent);

        distPanel->addKnob ("DRIVE", cerberus::params::distDrive, formatDb);
        distPanel->addKnob ("TONE", cerberus::params::distTone, formatTone);
        distPanel->addKnob ("MIX", cerberus::params::distMix, formatPercent);
        distPanel->addKnob ("OUTPUT", cerberus::params::distOutput, formatDb);

        auto sweep = std::make_unique<SweepMeter> (wah);
        sweepMeter = sweep.get();
        wahPanel->setBottomComponent (std::move (sweep), 40);

        auto reduction = std::make_unique<GainReductionMeter> (compressor);
        gainReductionMeter = reduction.get();
        compPanel->setBottomComponent (std::move (reduction), 40);

        auto modes = std::make_unique<ModeBar> (apvts, cerberus::params::distMode, distortion);
        modeBar = modes.get();
        distPanel->setBottomComponent (std::move (modes), 38);

        inputKnob = std::make_unique<ParamKnob> (apvts, cerberus::params::inputGain,
                                                 "INPUT", formatDb, globalAccent);
        addAndMakeVisible (*inputKnob);

        outputKnob = std::make_unique<ParamKnob> (apvts, cerberus::params::outputGain,
                                                  "OUTPUT", formatDb, globalAccent);
        addAndMakeVisible (*outputKnob);

        addAndMakeVisible (outputMeter);

        prevButton.setButtonText (juce::String::fromUTF8 ("\xe2\x80\xb9"));
        nextButton.setButtonText (juce::String::fromUTF8 ("\xe2\x80\xba"));
        prevButton.setTooltip ("Previous factory preset");
        nextButton.setTooltip ("Next factory preset");
        prevButton.onClick = [this] { stepPreset (-1); };
        nextButton.onClick = [this] { stepPreset (1); };
        addAndMakeVisible (prevButton);
        addAndMakeVisible (nextButton);

        presetBox.setTooltip ("Select a factory preset");
        presetBox.setJustificationType (juce::Justification::centredLeft);
        for (int i = 0; i < processor.getNumPrograms(); ++i)
            presetBox.addItem (processor.getProgramName (i), i + 1);

        presetBox.setSelectedId (processor.getCurrentPresetIndex() + 1, juce::dontSendNotification);
        presetBox.onChange = [this]
        {
            if (syncingPreset)
                return;

            const int selected = presetBox.getSelectedId();

            if (selected > 0)
            {
                processor.setCurrentProgram (selected - 1);
                lastPreset = selected - 1;
                lastDirty = processor.isPresetDirty();
                repaint (820, 14, 332, 64);
            }
        };
        addAndMakeVisible (presetBox);

        lastPreset = processor.getCurrentPresetIndex();
        lastDirty = processor.isPresetDirty();

        startTimerHz (30);
    }

    ~CerberusEditorContent() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient base (backgroundLift, bounds.getTopLeft(),
                                   background, bounds.getBottomLeft(), false);
        base.addColour (0.48, juce::Colour (0xff0a0e13));
        g.setGradientFill (base);
        g.fillAll();

        g.setGradientFill (juce::ColourGradient (wah.withAlpha (0.085f), 80.0f, -60.0f,
                                                 wah.withAlpha (0.0f), 680.0f, 360.0f, true));
        g.fillEllipse (-160.0f, -300.0f, 900.0f, 620.0f);

        g.setGradientFill (juce::ColourGradient (distortion.withAlpha (0.075f), 1130.0f, -100.0f,
                                                 distortion.withAlpha (0.0f), 760.0f, 300.0f, true));
        g.fillEllipse (460.0f, -330.0f, 900.0f, 600.0f);

        g.setGradientFill (juce::ColourGradient (compressor.withAlpha (0.045f), 610.0f, -160.0f,
                                                 compressor.withAlpha (0.0f), 600.0f, 420.0f, true));
        g.fillEllipse (260.0f, -320.0f, 720.0f, 640.0f);

        g.setColour (gridLine);
        for (int x = 28; x < designWidth - 20; x += 32)
            g.drawVerticalLine (x, 112.0f, 590.0f);

        for (int y = 130; y < 590; y += 32)
            g.drawHorizontalLine (y, 24.0f, static_cast<float> (designWidth - 24));

        // Header mark: three bars for three stages.
        const juce::Colour markColours[] { wah, compressor, distortion };
        for (int i = 0; i < 3; ++i)
        {
            const float height = i == 1 ? 38.0f : 27.0f;
            const float x = 28.0f + static_cast<float> (i) * 9.0f;
            const float top = 66.0f - height;
            g.setColour (markColours[i].withAlpha (0.88f));
            g.fillRoundedRectangle (x, top, 5.0f, height, 2.5f);
        }

        g.setColour (text);
        g.setFont (font (29.0f, FontWeight::black, 0.035f));
        g.drawText ("CERBERUS", 58, 20, 340, 38, juce::Justification::centredLeft, false);

        g.setColour (textMuted);
        g.setFont (font (8.5f, FontWeight::semibold, 0.24f));
        g.drawText ("AUTO-WAH    /    COMPRESSOR    /    DISTORTION",
                    59, 57, 420, 14, juce::Justification::centredLeft, false);

        g.setFont (font (8.0f, FontWeight::semibold, 0.15f));
        g.setColour (textMuted);
        g.drawText ("FACTORY PRESET", 872, 17, 160, 14,
                    juce::Justification::centredLeft, false);

        if (lastDirty)
        {
            g.setColour (amber.withAlpha (0.16f));
            g.fillRoundedRectangle (1056.0f, 16.0f, 52.0f, 16.0f, 4.0f);
            g.setColour (amber);
            g.setFont (font (7.5f, FontWeight::semibold, 0.13f));
            g.drawText ("EDITED", 1056, 16, 52, 16, juce::Justification::centred, false);
        }

        juce::ColourGradient headerLine (wah.withAlpha (0.75f), 28.0f, 102.0f,
                                         compressor.withAlpha (0.75f), 590.0f, 102.0f, false);
        headerLine.addColour (0.66, distortion.withAlpha (0.75f));
        g.setGradientFill (headerLine);
        g.fillRect (juce::Rectangle<float> (28.0f, 102.0f,
                                            static_cast<float> (designWidth - 56), 1.0f));

        // Stage connectors between the cards.
        for (const float x : { 399.5f, 780.5f })
        {
            g.setColour (juce::Colours::white.withAlpha (0.16f));
            juce::Path arrow;
            arrow.startNewSubPath (x - 3.0f, 326.0f);
            arrow.lineTo (x + 3.0f, 332.0f);
            arrow.lineTo (x - 3.0f, 338.0f);
            g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        // Footer surface.
        const auto footer = juce::Rectangle<float> (28.0f, 598.0f, 1124.0f, 94.0f);
        g.setColour (panelDeep.withAlpha (0.72f));
        g.fillRoundedRectangle (footer, 12.0f);
        g.setColour (border);
        g.drawRoundedRectangle (footer.reduced (0.5f), 12.0f, 1.0f);

        g.setColour (border);
        g.fillRect (juce::Rectangle<float> (236.0f, 612.0f, 1.0f, 66.0f));

        g.setFont (font (7.5f, FontWeight::semibold, 0.16f));
        g.setColour (textDim);
        g.drawText ("SERIAL SIGNAL PATH", 596, 611, 260, 12,
                    juce::Justification::centredLeft, false);

        const std::array<std::pair<const char*, juce::Colour>, 3> stages
        {
            std::pair { "WAH", wah },
            std::pair { "COMP", compressor },
            std::pair { "DRIVE", distortion }
        };

        float chipX = 596.0f;
        for (size_t i = 0; i < stages.size(); ++i)
        {
            const auto chip = juce::Rectangle<float> (chipX, 628.0f, 62.0f, 22.0f);
            g.setColour (stages[i].second.withAlpha (0.08f));
            g.fillRoundedRectangle (chip, 4.5f);
            g.setColour (stages[i].second.withAlpha (0.30f));
            g.drawRoundedRectangle (chip.reduced (0.5f), 4.0f, 1.0f);
            g.setFont (font (8.0f, FontWeight::semibold, 0.12f));
            g.setColour (stages[i].second.withAlpha (0.94f));
            g.drawText (stages[i].first, chip, juce::Justification::centred, false);

            if (i + 1 < stages.size())
            {
                g.setColour (textDim);
                g.setFont (font (10.0f, FontWeight::regular));
                g.drawText (juce::String::fromUTF8 ("\xe2\x80\xba"),
                            chip.getRight(), 627.0f, 16.0f, 24.0f,
                            juce::Justification::centred, false);
            }

            chipX += 78.0f;
        }

        g.setFont (font (8.5f, FontWeight::regular, 0.02f));
        g.setColour (textDim);
        g.drawText ("Three stages. One bite.", 596, 658, 220, 14,
                    juce::Justification::centredLeft, false);

        g.setFont (font (7.5f, FontWeight::semibold, 0.15f));
        g.setColour (textDim);
        g.drawText ("64-BIT VST3", 828, 611, 316, 12,
                    juce::Justification::centredRight, false);
        g.drawText ("BLS AUDIO  /  v1.0.0", 828, 630, 316, 14,
                    juce::Justification::centredRight, false);
        g.setFont (font (7.5f, FontWeight::regular, 0.04f));
        g.drawText ("Double-click a knob to reset", 828, 656, 316, 14,
                    juce::Justification::centredRight, false);
    }

    void resized() override
    {
        wahPanel->setBounds (28, 118, 362, 462);
        compPanel->setBounds (409, 118, 362, 462);
        distPanel->setBounds (790, 118, 362, 462);

        prevButton.setBounds (832, 40, 32, 32);
        presetBox.setBounds (872, 40, 232, 32);
        nextButton.setBounds (1112, 40, 32, 32);

        inputKnob->setBounds (40, 604, 86, 82);
        outputKnob->setBounds (136, 604, 86, 82);
        outputMeter.setBounds (258, 618, 302, 56);
    }

private:
    void stepPreset (int direction)
    {
        const int count = processor.getNumPrograms();

        if (count <= 0)
            return;

        const int current = processor.getCurrentPresetIndex();
        const int next = (current + direction + count) % count;
        processor.setCurrentProgram (next);
        syncingPreset = true;
        presetBox.setSelectedId (next + 1, juce::dontSendNotification);
        syncingPreset = false;
        lastPreset = next;
        lastDirty = processor.isPresetDirty();
        repaint (820, 14, 332, 64);
    }

    void timerCallback() override
    {
        outputMeter.setLevels (processor.getOutputLevelLeft(), processor.getOutputLevelRight());

        if (gainReductionMeter != nullptr)
            gainReductionMeter->setReductionDb (processor.getCompressorGainReductionDb());

        if (sweepMeter != nullptr)
            sweepMeter->setCutoffHz (processor.getWahCutoffHz());

        const int preset = processor.getCurrentPresetIndex();
        const bool dirty = processor.isPresetDirty();
        bool needsRepaint = false;

        if (preset != lastPreset)
        {
            syncingPreset = true;
            presetBox.setSelectedId (preset + 1, juce::dontSendNotification);
            syncingPreset = false;
            lastPreset = preset;
            needsRepaint = true;
        }

        if (dirty != lastDirty)
        {
            lastDirty = dirty;
            needsRepaint = true;
        }

        if (needsRepaint)
            repaint (820, 14, 332, 64);
    }

    CerberusAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<EffectPanel> wahPanel;
    std::unique_ptr<EffectPanel> compPanel;
    std::unique_ptr<EffectPanel> distPanel;

    std::unique_ptr<ParamKnob> inputKnob;
    std::unique_ptr<ParamKnob> outputKnob;
    StereoMeter outputMeter;

    juce::TextButton prevButton;
    juce::TextButton nextButton;
    juce::ComboBox presetBox;

    SweepMeter* sweepMeter = nullptr;
    GainReductionMeter* gainReductionMeter = nullptr;
    ModeBar* modeBar = nullptr;

    int lastPreset = 0;
    bool lastDirty = false;
    bool syncingPreset = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusEditorContent)
};

//==============================================================================
CerberusAudioProcessorEditor::CerberusAudioProcessorEditor (CerberusAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    content = std::make_unique<CerberusEditorContent> (processor);
    addAndMakeVisible (*content);

    setResizable (true, true);
    setResizeLimits (885, 540, 1770, 1080);

    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (static_cast<double> (designWidth) / static_cast<double> (designHeight));

    setSize (designWidth, designHeight);
}

CerberusAudioProcessorEditor::~CerberusAudioProcessorEditor()
{
    content.reset();
    setLookAndFeel (nullptr);
}

void CerberusAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void CerberusAudioProcessorEditor::resized()
{
    if (content == nullptr)
        return;

    const float scale = juce::jmin (static_cast<float> (getWidth()) / static_cast<float> (designWidth),
                                    static_cast<float> (getHeight()) / static_cast<float> (designHeight));
    const float offsetX = (static_cast<float> (getWidth()) - static_cast<float> (designWidth) * scale) * 0.5f;
    const float offsetY = (static_cast<float> (getHeight()) - static_cast<float> (designHeight) * scale) * 0.5f;

    content->setBounds (0, 0, designWidth, designHeight);
    content->setTransform (juce::AffineTransform::scale (scale).translated (offsetX, offsetY));
}
