#pragma once

#include <JuceHeader.h>

namespace bassforge::gui
{
/** Dark, neon-accented look for the synth - flat panels and glowing knobs. */
class BassForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr juce::uint32 cBackground = 0xff14161c;
    static constexpr juce::uint32 cPanel      = 0xff1e222c;
    static constexpr juce::uint32 cPanelEdge  = 0xff2c3140;
    static constexpr juce::uint32 cAccent     = 0xff31d0aa; // teal
    static constexpr juce::uint32 cAccent2    = 0xffff7a45; // orange
    static constexpr juce::uint32 cText       = 0xffd7dbe4;
    static constexpr juce::uint32 cTextDim    = 0xff8b93a4;

    BassForgeLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (cBackground));
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (cText));
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (cPanel));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (cText));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (cPanel));
        setColour (juce::ComboBox::textColourId, juce::Colour (cText));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (cPanelEdge));
        setColour (juce::ComboBox::arrowColourId, juce::Colour (cAccent));
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (cPanel));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (cAccent).withAlpha (0.3f));
        setColour (juce::PopupMenu::textColourId, juce::Colour (cText));
        setColour (juce::ToggleButton::textColourId, juce::Colour (cText));
        setColour (juce::ToggleButton::tickColourId, juce::Colour (cAccent));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float track  = radius * 0.82f;
        const float lineW  = juce::jmax (2.5f, radius * 0.14f);

        // Track background
        juce::Path bg;
        bg.addCentredArc (centre.x, centre.y, track, track, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (cPanelEdge));
        g.strokePath (bg, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        juce::Path arc;
        if (bipolar)
        {
            const float midAngle = startAngle + 0.5f * (endAngle - startAngle);
            arc.addCentredArc (centre.x, centre.y, track, track, 0.0f, midAngle, angle, true);
        }
        else
        {
            arc.addCentredArc (centre.x, centre.y, track, track, 0.0f, startAngle, angle, true);
        }
        g.setColour (juce::Colour (cAccent));
        g.strokePath (arc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Knob body
        const float bodyR = track * 0.66f;
        g.setColour (juce::Colour (cPanel).brighter (0.15f));
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (juce::Colour (cPanelEdge));
        g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        // Pointer
        juce::Path p;
        const float pointerLen = bodyR * 0.9f;
        p.addRoundedRectangle (-lineW * 0.35f, -pointerLen, lineW * 0.7f, pointerLen * 0.7f, lineW * 0.35f);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
        g.setColour (juce::Colour (cAccent).brighter (0.4f));
        g.fillPath (p);
    }

    juce::Font getLabelFont (juce::Label& label) override
    {
        return juce::Font (juce::jmin (13.0f, (float) label.getHeight() * 0.9f));
    }
};
} // namespace bassforge::gui
