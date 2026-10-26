#include "PluginEditor.h"

#include "Parameters.h"
#include "ui/Controls.h"
#include "ui/EffectPanel.h"
#include "ui/Theme.h"
#include "ui/WaveformDisplay.h"

#include <array>
#include <cmath>
#include <utility>

using namespace cerberus::ui;

namespace
{
constexpr int designWidth = 1180;
constexpr int designHeight = 720;

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
                                                  crimsonAccent, cerberus::params::wahOn, 3, "WAH");
        addAndMakeVisible (*wahPanel);

        compPanel = std::make_unique<EffectPanel> (apvts, "02", "COMPRESSOR",
                                                   "FEED-FORWARD DYNAMICS",
                                                   crimsonAccent, cerberus::params::compOn, 3, "COMP");
        addAndMakeVisible (*compPanel);

        distPanel = std::make_unique<EffectPanel> (apvts, "03", "DISTORTION",
                                                   "4x OVERSAMPLED SATURATION",
                                                   crimsonAccent, cerberus::params::distOn, 2, "DRIVE");
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

        sweepMeter = new SweepMeter();
        wahPanel->setBottomComponent (std::unique_ptr<SweepMeter> (sweepMeter), 34);

        gainReductionMeter = new GainReductionMeter();
        compPanel->setBottomComponent (std::unique_ptr<GainReductionMeter> (gainReductionMeter), 34);

        modeBar = new ModeBar (apvts, cerberus::params::distMode);
        distPanel->setBottomComponent (std::unique_ptr<ModeBar> (modeBar), 34);

        inputSlider = std::make_unique<ParamSlider> (apvts, cerberus::params::inputGain,
                                                     "INPUT", formatDb);
        addAndMakeVisible (*inputSlider);

        outputSlider = std::make_unique<ParamSlider> (apvts, cerberus::params::outputGain,
                                                      "OUTPUT", formatDb);
        addAndMakeVisible (*outputSlider);

        addAndMakeVisible (outputMeter);

        waveform = std::make_unique<WaveformDisplay> (processor);
        addAndMakeVisible (*waveform);

        presetPill = std::make_unique<PresetPill> (processor);
        addAndMakeVisible (*presetPill);

        startTimerHz (30);
    }

    ~CerberusEditorContent() override
    {
        stopTimer();
    }

    void paint (juce::Graphics& g) override
    {
        drawDesktopHardwareChassis (g, getLocalBounds().toFloat());

        paintHeader (g);
        paintFooter (g);
    }

    void resized() override
    {
        presetPill->setBounds (375, 30, 430, 44);

        waveform->setBounds (30, 88, 1118, 128);

        wahPanel->setBounds (30, 226, 360, 364);
        compPanel->setBounds (409, 226, 360, 364);
        distPanel->setBounds (788, 226, 360, 364);

        inputSlider->setBounds (44, 604, 58, 80);
        outputSlider->setBounds (108, 604, 58, 80);
        outputMeter.setBounds (196, 610, 300, 66);
    }

private:
    void paintHeader (juce::Graphics& g)
    {
        // Three crimson stage bars as the Cerberus mark.
        const std::array<float, 3> heights { 18.0f, 27.0f, 21.0f };

        for (size_t i = 0; i < heights.size(); ++i)
        {
            const float x = 42.0f + static_cast<float> (i) * 8.0f;
            const float top = 74.0f - heights[i];
            const auto bar = juce::Rectangle<float> (x, top, 4.5f, heights[i]);

            g.setColour (crimsonGlow.withAlpha (0.32f));
            g.fillRoundedRectangle (bar.expanded (1.2f, 0.8f), 3.0f);
            g.setColour (crimsonAccent);
            g.fillRoundedRectangle (bar, 2.2f);
            g.setColour (crimsonBright.withAlpha (0.85f));
            g.fillRoundedRectangle (bar.reduced (1.0f, 1.4f), 1.0f);
        }

        g.setColour (textPrimary);
        g.setFont (font (25.0f, FontWeight::black, 0.03f));
        g.drawText ("CERBERUS", 76, 28, 280, 30, juce::Justification::centredLeft, false);

        g.setColour (textSecondary);
        g.setFont (font (7.0f, FontWeight::semibold, 0.17f));
        g.drawText ("AUTO-WAH  /  COMPRESSOR  /  DISTORTION",
                    77, 57, 280, 10, juce::Justification::centredLeft, false);

        g.setColour (textSecondary.withAlpha (0.62f));
        g.setFont (font (6.2f, FontWeight::semibold, 0.16f));
        g.drawText ("THREE-STAGE ANALOG BITE", 77, 68, 280, 9,
                    juce::Justification::centredLeft, false);

        g.setColour (textPrimary.withAlpha (0.90f));
        g.setFont (font (7.2f, FontWeight::black, 0.18f));
        g.drawText ("BLS AUDIO", 930, 36, 176, 10,
                    juce::Justification::centredRight, false);

        g.setColour (textMuted);
        g.setFont (font (6.2f, FontWeight::semibold, 0.14f));
        g.drawText ("64-BIT VST3  /  4x OS", 930, 49, 176, 9,
                    juce::Justification::centredRight, false);
        g.setColour (textSecondary.withAlpha (0.58f));
        g.drawText ("SERIES CERBERUS", 930, 62, 176, 9,
                    juce::Justification::centredRight, false);
    }

    void paintFooter (juce::Graphics& g)
    {
        const auto footer = juce::Rectangle<float> (30.0f, 600.0f, 1118.0f, 88.0f);

        drawGlassWell (g, footer, 7.0f);

        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.025f), footer.getTopLeft(),
                                    juce::Colours::transparentWhite, footer.getBottomLeft(), false);
        g.setGradientFill (sheen);
        g.fillRoundedRectangle (footer, 7.0f);

        g.setColour (glassRimHighlight.withAlpha (0.14f));
        g.fillRect (footer.getX() + 140.0f, footer.getY() + 11.0f, 1.0f, footer.getHeight() - 22.0f);

        // Serial signal-path readout.
        g.setColour (textMuted);
        g.setFont (font (6.4f, FontWeight::semibold, 0.17f));
        g.drawText ("SERIAL SIGNAL PATH", 548, 608, 260, 10,
                    juce::Justification::centredLeft, false);

        const std::array<const char*, 4> stages { "AUTO-WAH", "COMP", "DRIVE", "OUT" };

        float chipX = 548.0f;

        for (size_t i = 0; i < stages.size(); ++i)
        {
            const auto chip = juce::Rectangle<float> (chipX, 624.0f, 74.0f, 19.0f);
            drawSignalChip (g, chip, stages[i]);

            if (i + 1 < stages.size())
            {
                g.setColour (crimsonAccent.withAlpha (0.72f));
                g.setFont (font (8.0f, FontWeight::black));
                g.drawText (">", juce::Rectangle<float> (chip.getRight(), chip.getY(),
                                                         12.0f, chip.getHeight()),
                            juce::Justification::centred, false);
            }

            chipX += 86.0f;
        }

        g.setColour (textMuted);
        g.setFont (font (6.0f, FontWeight::semibold, 0.13f));
        g.drawText ("TRUE THREE-STAGE SIGNAL PATH  /  INDEPENDENT MODULE BYPASS",
                    548, 650, 380, 9, juce::Justification::centredLeft, false);
        g.drawText ("4x OVERSAMPLED SATURATION  /  CONSTANT LATENCY",
                    548, 663, 380, 9, juce::Justification::centredLeft, false);

        g.setColour (textPrimary.withAlpha (0.92f));
        g.setFont (font (7.0f, FontWeight::black, 0.14f));
        g.drawText ("BLS AUDIO", 930, 610, 176, 10,
                    juce::Justification::centredRight, false);

        g.setColour (textMuted);
        g.setFont (font (6.2f, FontWeight::semibold, 0.13f));
        g.drawText ("64-BIT FLOAT  /  v1.0.0", 930, 625, 176, 9,
                    juce::Justification::centredRight, false);
        g.drawText ("Double-click a knob or slider to reset", 930, 650, 176, 9,
                    juce::Justification::centredRight, false);
        g.drawText ("BLS AUDIO  /  CERBERUS", 930, 663, 176, 9,
                    juce::Justification::centredRight, false);
    }

    void timerCallback() override
    {
        outputMeter.setLevels (processor.getOutputLevelLeft(), processor.getOutputLevelRight());

        if (gainReductionMeter != nullptr)
            gainReductionMeter->setReductionDb (processor.getCompressorGainReductionDb());

        if (sweepMeter != nullptr)
            sweepMeter->setCutoffHz (processor.getWahCutoffHz());

        if (presetPill != nullptr)
            presetPill->refresh();
    }

    CerberusAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

    std::unique_ptr<EffectPanel> wahPanel;
    std::unique_ptr<EffectPanel> compPanel;
    std::unique_ptr<EffectPanel> distPanel;

    std::unique_ptr<ParamSlider> inputSlider;
    std::unique_ptr<ParamSlider> outputSlider;
    StereoMeter outputMeter;

    std::unique_ptr<WaveformDisplay> waveform;
    std::unique_ptr<PresetPill> presetPill;

    SweepMeter* sweepMeter = nullptr;
    GainReductionMeter* gainReductionMeter = nullptr;
    ModeBar* modeBar = nullptr;

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
    g.fillAll (studioSurfaceDark);
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
