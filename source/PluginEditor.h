#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/BassForgeLookAndFeel.h"
#include "gui/ParamControl.h"
#include "gui/SectionPanel.h"

class BassForgeAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit BassForgeAudioProcessorEditor (BassForgeAudioProcessor&);
    ~BassForgeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    bassforge::gui::SectionPanel& addSection (const juce::String& title, int columns);

    BassForgeAudioProcessor& processor;
    bassforge::gui::BassForgeLookAndFeel lnf;

    juce::OwnedArray<bassforge::gui::SectionPanel> sections;

    float meterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassForgeAudioProcessorEditor)
};
