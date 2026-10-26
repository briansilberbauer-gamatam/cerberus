#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <cmath>
#include <iostream>
#include <memory>

class CerberusRenderApplication final : public juce::JUCEApplication,
                                        private juce::Timer
{
public:
    const juce::String getApplicationName() override { return "Cerberus Screenshot"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}
    void systemRequestedQuit() override {}
    void shutdown() override {}

    void initialise (const juce::String& commandLine) override
    {
        outputFile = juce::File::getCurrentWorkingDirectory().getChildFile ("cerberus-ui.png");

        for (const auto& argument : juce::StringArray::fromTokens (commandLine, true))
        {
            if (argument.startsWith ("--out="))
            {
                const auto path = argument.substring (6);
                outputFile = juce::File::isAbsolutePath (path)
                                 ? juce::File (path)
                                 : juce::File::getCurrentWorkingDirectory().getChildFile (path);
            }
        }

        outputFile = outputFile.getFullPathName();
        outputFile.getParentDirectory().createDirectory();

        processor = std::make_unique<CerberusAudioProcessor>();
        editor = std::unique_ptr<juce::AudioProcessorEditor> (processor->createEditor());

        if (editor == nullptr)
        {
            std::cerr << "Could not create the plugin editor.\n";
            setApplicationReturnValue (1);
            quit();
            return;
        }

        editor->setSize (1180, 720);
        editor->addToDesktop (0);
        editor->setTopLeftPosition (30, 30);
        editor->setVisible (true);
        editor->toFront (true);

        // Park the cursor far away from the unit so no hover tooltip is captured.
        juce::Desktop::getInstance().getMainMouseSource().setScreenPosition ({ 3000.0f, 3000.0f });

        processor->setPlayConfigDetails (2, 2, 48000.0, 1024);
        processor->prepareToPlay (48000.0, 1024);
        testBuffer.setSize (2, 1024);
        startTimerHz (30);

        juce::Timer::callAfterDelay (1500, [this]
        {
            stopTimer();
            juce::Desktop::getInstance().getMainMouseSource().setScreenPosition ({ 3000.0f, 3000.0f });

            // A stale hover tooltip may still be visible from editor construction.
            auto hideTooltips = [&] (auto&& self, juce::Component& component) -> void
            {
                for (auto* child : component.getChildren())
                {
                    if (dynamic_cast<juce::TooltipWindow*> (child) != nullptr)
                        child->setVisible (false);
                    else
                        self (self, *child);
                }
            };
            hideTooltips (hideTooltips, *editor);

            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

            if (image.isValid())
            {
                outputFile.deleteFile();

                if (auto stream = std::unique_ptr<juce::FileOutputStream> (outputFile.createOutputStream()))
                {
                    juce::PNGImageFormat png;
                    const bool written = png.writeImageToStream (image, *stream);
                    stream->flush();

                    if (written)
                        std::cout << "Rendered " << outputFile.getFullPathName() << "\n";
                    else
                        std::cerr << "Failed to encode PNG.\n";
                }
                else
                {
                    std::cerr << "Could not open " << outputFile.getFullPathName() << "\n";
                }
            }
            else
            {
                std::cerr << "Editor snapshot was invalid.\n";
            }

            processor->releaseResources();
            editor->removeFromDesktop();
            editor.reset();
            processor.reset();
            quit();
        });
    }

private:
    void timerCallback() override
    {
        if (processor == nullptr)
            return;

        for (int i = 0; i < testBuffer.getNumSamples(); ++i)
        {
            const double t = static_cast<double> (sampleCounter + i) / 48000.0;
            const float movement = 0.45f + 0.35f * static_cast<float> (
                std::sin (juce::MathConstants<double>::twoPi * 1.7 * t));
            const double fundamental = juce::MathConstants<double>::twoPi * 660.0 * t;
            const float signal = 0.40f * movement * static_cast<float> (
                std::sin (fundamental)
                + 0.55 * std::sin (2.0 * fundamental)
                + 0.35 * std::sin (3.0 * fundamental)
                + 0.20 * std::sin (4.0 * fundamental));

            for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch)
                testBuffer.setSample (ch, i, ch == 0 ? signal : signal * 0.92f);
        }

        sampleCounter += testBuffer.getNumSamples();
        juce::MidiBuffer midi;
        processor->processBlock (testBuffer, midi);
    }

    std::unique_ptr<CerberusAudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::File outputFile;
    juce::AudioBuffer<float> testBuffer;
    juce::int64 sampleCounter = 0;
};

START_JUCE_APPLICATION (CerberusRenderApplication)
