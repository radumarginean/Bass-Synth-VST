#pragma once

#include <JuceHeader.h>
#include "ParamControl.h"
#include "BassForgeLookAndFeel.h"

namespace bassforge::gui
{
/**
    A titled panel that lays out its ParamControls in a fixed-column grid.
*/
class SectionPanel : public juce::Component
{
public:
    SectionPanel (juce::AudioProcessorValueTreeState& state, juce::String sectionTitle, int cols)
        : apvts (state), title (std::move (sectionTitle)), columns (cols) {}

    void add (const juce::String& paramId, const juce::String& caption)
    {
        auto* c = controls.add (new ParamControl (apvts, paramId, caption));
        addAndMakeVisible (c);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (BassForgeLookAndFeel::cPanel));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colour (BassForgeLookAndFeel::cPanelEdge));
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (juce::Colour (BassForgeLookAndFeel::cAccent));
        g.fillRoundedRectangle (r.removeFromTop (headerHeight).reduced (1.0f).withTrimmedBottom (-2.0f), 8.0f);
        g.setColour (juce::Colour (0xff10131a));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (title, getLocalBounds().removeFromTop (headerHeight), juce::Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().withTrimmedTop (headerHeight).reduced (6);
        const int n = controls.size();
        if (n == 0) return;

        const int rows = (n + columns - 1) / columns;
        const int cellW = area.getWidth() / columns;
        const int cellH = area.getHeight() / juce::jmax (1, rows);

        for (int i = 0; i < n; ++i)
        {
            const int col = i % columns;
            const int row = i / columns;
            controls[i]->setBounds (area.getX() + col * cellW,
                                    area.getY() + row * cellH,
                                    cellW, cellH);
        }
    }

    int preferredRows() const { return (controls.size() + columns - 1) / columns; }

private:
    static constexpr int headerHeight = 22;

    juce::AudioProcessorValueTreeState& apvts;
    juce::String title;
    int columns = 4;
    juce::OwnedArray<ParamControl> controls;
};
} // namespace bassforge::gui
