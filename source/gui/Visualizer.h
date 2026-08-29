#pragma once

#include <JuceHeader.h>
#include "BassForgeLookAndFeel.h"
#include <functional>

namespace bassforge::gui
{
/**
    Combined oscilloscope + spectrum analyzer.

    Pulls the most recent output samples from the processor (via an injected
    reader), draws an FFT magnitude curve (filled) and a triggered waveform
    (line) on top. Click to cycle Both / Scope / Spectrum.
*/
class Visualizer : public juce::Component,
                   private juce::Timer
{
public:
    enum class Mode { both = 0, scope, spectrum };

    explicit Visualizer (std::function<void (float*, int)> reader)
        : scopeReader (std::move (reader)),
          fft (fftOrder),
          window (fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        setInterceptsMouseClicks (true, false);
        startTimerHz (30);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        mode = (Mode) (((int) mode + 1) % 3);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();

        g.setColour (juce::Colour (0xff0d0f14));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (juce::Colour (BassForgeLookAndFeel::cPanelEdge));
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

        auto area = r.reduced (6.0f);

        // Baseline grid
        g.setColour (juce::Colour (BassForgeLookAndFeel::cPanelEdge).withAlpha (0.5f));
        g.drawHorizontalLine ((int) area.getCentreY(), area.getX(), area.getRight());

        if (mode == Mode::both || mode == Mode::spectrum)
            drawSpectrum (g, area);
        if (mode == Mode::both || mode == Mode::scope)
            drawScope (g, area);

        // Mode label
        g.setColour (juce::Colour (BassForgeLookAndFeel::cTextDim));
        g.setFont (juce::Font (10.0f));
        const char* names[] = { "SCOPE + SPECTRUM", "SCOPE", "SPECTRUM" };
        g.drawText (names[(int) mode], area.removeFromTop (14).toNearestInt(),
                    juce::Justification::topRight);
    }

private:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;     // 2048
    static constexpr int scopeLen = 1024;

    void timerCallback() override { repaint(); }

    void drawScope (juce::Graphics& g, juce::Rectangle<float> area)
    {
        float samples[scopeLen];
        if (scopeReader) scopeReader (samples, scopeLen);
        else return;

        // Simple rising-edge trigger for a stable display.
        int trigger = 0;
        for (int i = 1; i < scopeLen / 2; ++i)
            if (samples[i - 1] < 0.0f && samples[i] >= 0.0f) { trigger = i; break; }

        const int span = scopeLen / 2;
        juce::Path path;
        for (int i = 0; i < span; ++i)
        {
            const int idx = juce::jmin (scopeLen - 1, trigger + i);
            const float x = area.getX() + (float) i / (float) (span - 1) * area.getWidth();
            const float y = area.getCentreY() - samples[idx] * area.getHeight() * 0.45f;
            if (i == 0) path.startNewSubPath (x, y);
            else        path.lineTo (x, y);
        }

        g.setColour (juce::Colour (BassForgeLookAndFeel::cAccent));
        g.strokePath (path, juce::PathStrokeType (1.6f));
    }

    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (scopeReader) scopeReader (fftData, fftSize);
        else return;

        window.multiplyWithWindowingTable (fftData, (size_t) fftSize);
        std::fill (fftData + fftSize, fftData + 2 * fftSize, 0.0f);
        fft.performFrequencyOnlyForwardTransform (fftData);

        const double sr = 44100.0; // display scaling only; exact rate not critical
        const float minF = 20.0f, maxF = 20000.0f;

        juce::Path path;
        path.startNewSubPath (area.getX(), area.getBottom());

        const int steps = juce::jmax (2, (int) area.getWidth());
        for (int s = 0; s < steps; ++s)
        {
            const float xNorm = (float) s / (float) (steps - 1);
            const float freq  = minF * std::pow (maxF / minF, xNorm);
            const float bin   = freq / (float) sr * fftSize;
            const int   b     = juce::jlimit (1, fftSize / 2 - 1, (int) bin);

            const float mag   = fftData[b] / (float) fftSize;
            float db          = juce::Decibels::gainToDecibels (mag + 1.0e-9f, -100.0f);
            const float level = juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f);

            const float x = area.getX() + xNorm * area.getWidth();
            const float y = area.getBottom() - level * area.getHeight();
            path.lineTo (x, y);
        }
        path.lineTo (area.getRight(), area.getBottom());
        path.closeSubPath();

        g.setColour (juce::Colour (BassForgeLookAndFeel::cAccent2).withAlpha (0.35f));
        g.fillPath (path);
        g.setColour (juce::Colour (BassForgeLookAndFeel::cAccent2).withAlpha (0.8f));
        g.strokePath (path, juce::PathStrokeType (1.0f));
    }

    std::function<void (float*, int)> scopeReader;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    float fftData[2 * fftSize] {};
    Mode  mode = Mode::both;
};
} // namespace bassforge::gui
