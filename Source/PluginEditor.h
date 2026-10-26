#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ui/StudioLookAndFeel.h"

#include <memory>

class CerberusEditorContent;

class CerberusAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit CerberusAudioProcessorEditor (CerberusAudioProcessor&);
    ~CerberusAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CerberusAudioProcessor& processor;
    cerberus::ui::StudioLookAndFeel lookAndFeel;
    std::unique_ptr<CerberusEditorContent> content;
    juce::TooltipWindow tooltipWindow { this, 650 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CerberusAudioProcessorEditor)
};
