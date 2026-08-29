#pragma once

#include <JuceHeader.h>
#include "BassForgeLookAndFeel.h"
#include "../Presets.h"

namespace bassforge::gui
{
/**
    Header strip for factory/user preset management: a browsable combo box,
    previous/next steppers, and save/load of the full plugin state to disk.
*/
class PresetBar : public juce::Component
{
public:
    PresetBar (juce::AudioProcessorValueTreeState& state, PresetManager& mgr)
        : apvts (state), manager (mgr)
    {
        combo.addItemList (manager.getPresetNames(), 1);
        combo.setSelectedItemIndex (manager.getCurrentIndex(), juce::dontSendNotification);
        combo.setJustificationType (juce::Justification::centredLeft);
        combo.onChange = [this]
        {
            const int idx = combo.getSelectedItemIndex();
            if (idx >= 0) manager.loadPreset (idx);
        };
        addAndMakeVisible (combo);

        prev.setButtonText ("<");
        next.setButtonText (">");
        prev.onClick = [this] { step (-1); };
        next.onClick = [this] { step (+1); };
        addAndMakeVisible (prev);
        addAndMakeVisible (next);

        save.setButtonText ("Save");
        load.setButtonText ("Load");
        save.onClick = [this] { savePreset(); };
        load.onClick = [this] { loadPreset(); };
        addAndMakeVisible (save);
        addAndMakeVisible (load);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (2);
        prev.setBounds (r.removeFromLeft (26));
        r.removeFromLeft (2);
        combo.setBounds (r.removeFromLeft (200));
        r.removeFromLeft (2);
        next.setBounds (r.removeFromLeft (26));

        load.setBounds (r.removeFromRight (56));
        r.removeFromRight (4);
        save.setBounds (r.removeFromRight (56));
    }

private:
    void step (int delta)
    {
        const int n = manager.getNumPresets();
        if (n == 0) return;
        int idx = (combo.getSelectedItemIndex() + delta + n) % n;
        combo.setSelectedItemIndex (idx, juce::sendNotification);
    }

    void savePreset()
    {
        chooser = std::make_unique<juce::FileChooser> (
            "Save BassForge preset", juce::File(), "*.bfpreset");

        const auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;

        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File()) return;
            file = file.withFileExtension ("bfpreset");
            if (auto xml = apvts.copyState().createXml())
                xml->writeTo (file);
        });
    }

    void loadPreset()
    {
        chooser = std::make_unique<juce::FileChooser> (
            "Load BassForge preset", juce::File(), "*.bfpreset");

        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (! file.existsAsFile()) return;
            if (auto xml = juce::XmlDocument::parse (file))
                if (xml->hasTagName (apvts.state.getType()))
                    apvts.replaceState (juce::ValueTree::fromXml (*xml));
        });
    }

    juce::AudioProcessorValueTreeState& apvts;
    PresetManager& manager;

    juce::ComboBox   combo;
    juce::TextButton prev, next, save, load;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace bassforge::gui
