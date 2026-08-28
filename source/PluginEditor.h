#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/BassForgeLookAndFeel.h"
#include "gui/ParamControl.h"
#include "gui/SectionPanel.h"
#include "gui/PresetBar.h"
#include "gui/Visualizer.h"

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
    std::unique_ptr<bassforge::gui::PresetBar>  presetBar;
    std::unique_ptr<bassforge::gui::Visualizer> visualizer;

    float meterLevel = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassForgeAudioProcessorEditor)
};
