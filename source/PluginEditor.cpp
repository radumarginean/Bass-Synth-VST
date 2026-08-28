#include "PluginEditor.h"
#include "dsp/Parameters.h"

using namespace bassforge;
using namespace bassforge::gui;

namespace
{
// A row in the section grid: the panels, their horizontal weights, and the
// row's vertical weight.
struct Row
{
    std::vector<SectionPanel*> panels;
    std::vector<float>         widths;
    float                      height;
};
}

BassForgeAudioProcessorEditor::BassForgeAudioProcessorEditor (BassForgeAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    auto& state = processor.getValueTreeState();

    // ---- OSC 1 ----
    {
        auto& s = addSection ("OSC 1", 4);
        s.add (pid::osc1Wave, "Wave");   s.add (pid::osc1Oct, "Oct");
        s.add (pid::osc1Semi, "Semi");   s.add (pid::osc1Fine, "Fine");
        s.add (pid::osc1Level, "Level"); s.add (pid::osc1Pw, "PW");
        s.add (pid::osc1Wt, "WT Pos");
    }
    // ---- OSC 2 ----
    {
        auto& s = addSection ("OSC 2", 4);
        s.add (pid::osc2Wave, "Wave");   s.add (pid::osc2Oct, "Oct");
        s.add (pid::osc2Semi, "Semi");   s.add (pid::osc2Fine, "Fine");
        s.add (pid::osc2Level, "Level"); s.add (pid::osc2Pw, "PW");
        s.add (pid::osc2Wt, "WT Pos");   s.add (pid::osc2Sync, "Sync");
    }
    // ---- SUB / NOISE ----
    {
        auto& s = addSection ("SUB / NOISE", 4);
        s.add (pid::subWave, "Sub Wave"); s.add (pid::subOct, "Sub Oct");
        s.add (pid::subLevel, "Sub");     s.add (pid::noiseLevel, "Noise");
    }
    // ---- FILTER ----
    {
        auto& s = addSection ("FILTER", 3);
        s.add (pid::filterMode, "Mode");   s.add (pid::cutoff, "Cutoff");
        s.add (pid::resonance, "Reso");    s.add (pid::filterDrv, "Drive");
        s.add (pid::keyTrack, "Key Trk");  s.add (pid::filterEnvAmt, "Env Amt");
    }
    // ---- UNISON ----
    {
        auto& s = addSection ("UNISON", 3);
        s.add (pid::uniVoices, "Voices"); s.add (pid::uniDetune, "Detune");
        s.add (pid::uniWidth, "Width");
    }
    // ---- DRIVE ----
    {
        auto& s = addSection ("DRIVE", 2);
        s.add (pid::drive, "Amount"); s.add (pid::driveType, "Type");
    }
    // ---- AMP ENV ----
    {
        auto& s = addSection ("AMP ENV", 4);
        s.add (pid::ampA, "A"); s.add (pid::ampD, "D");
        s.add (pid::ampS, "S"); s.add (pid::ampR, "R");
    }
    // ---- FILTER ENV ----
    {
        auto& s = addSection ("FILTER ENV", 4);
        s.add (pid::fegA, "A"); s.add (pid::fegD, "D");
        s.add (pid::fegS, "S"); s.add (pid::fegR, "R");
    }
    // ---- LFO 1 ----
    {
        auto& s = addSection ("LFO 1", 3);
        s.add (pid::lfo1Shape, "Shape"); s.add (pid::lfo1Rate, "Rate");
        s.add (pid::lfo1Sync, "Sync");
    }
    // ---- LFO 2 ----
    {
        auto& s = addSection ("LFO 2", 3);
        s.add (pid::lfo2Shape, "Shape"); s.add (pid::lfo2Rate, "Rate");
        s.add (pid::lfo2Sync, "Sync");
    }
    // ---- MOD MATRIX ----
    {
        auto& s = addSection ("MOD MATRIX", 3);
        for (int i = 0; i < numModSlots; ++i)
        {
            s.add (pid::modSrc (i), "Source");
            s.add (pid::modDst (i), "Dest");
            s.add (pid::modAmt (i), "Amount");
        }
    }
    // ---- GLOBAL ----
    {
        auto& s = addSection ("GLOBAL", 2);
        s.add (pid::voiceMode, "Mode");  s.add (pid::glide, "Glide");
        s.add (pid::bendRange, "Bend");  s.add (pid::masterVol, "Master");
    }

    setSize (1120, 760);
    startTimerHz (24);
}

BassForgeAudioProcessorEditor::~BassForgeAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

SectionPanel& BassForgeAudioProcessorEditor::addSection (const juce::String& title, int columns)
{
    auto* s = new SectionPanel (processor.getValueTreeState(), title, columns);
    sections.add (s);
    addAndMakeVisible (s);
    return *s;
}

void BassForgeAudioProcessorEditor::timerCallback()
{
    const float lvl = processor.outputLevel.load();
    if (std::abs (lvl - meterLevel) > 0.001f)
    {
        meterLevel = meterLevel * 0.6f + lvl * 0.4f;
        repaint (0, 0, getWidth(), 46);
    }
}

void BassForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (BassForgeLookAndFeel::cBackground));

    // Header bar
    auto header = getLocalBounds().removeFromTop (46);
    g.setColour (juce::Colour (BassForgeLookAndFeel::cPanel));
    g.fillRect (header);

    auto title = header.reduced (16, 0);
    g.setColour (juce::Colour (BassForgeLookAndFeel::cAccent));
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    g.drawText ("BASS", title.removeFromLeft (72), juce::Justification::centredLeft);
    g.setColour (juce::Colour (BassForgeLookAndFeel::cText));
    g.drawText ("FORGE", title.removeFromLeft (86), juce::Justification::centredLeft);

    g.setColour (juce::Colour (BassForgeLookAndFeel::cTextDim));
    g.setFont (juce::Font (11.0f));
    g.drawText ("bass synth  \xe2\x80\xa2  wavetable + ladder", title.removeFromLeft (260),
                juce::Justification::centredLeft);

    // Output meter
    auto meter = header.removeFromRight (200).reduced (16, 16);
    g.setColour (juce::Colour (BassForgeLookAndFeel::cPanelEdge));
    g.fillRoundedRectangle (meter.toFloat(), 3.0f);
    const float w = meter.getWidth() * juce::jlimit (0.0f, 1.0f, meterLevel);
    auto fill = meter.toFloat().withWidth (w);
    juce::Colour mc = meterLevel > 0.9f ? juce::Colour (BassForgeLookAndFeel::cAccent2)
                                        : juce::Colour (BassForgeLookAndFeel::cAccent);
    g.setColour (mc);
    g.fillRoundedRectangle (fill, 3.0f);
}

void BassForgeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().withTrimmedTop (46).reduced (8);

    if (sections.size() < 12)
        return;

    // Named lookup by construction order
    auto* osc1   = sections[0];
    auto* osc2   = sections[1];
    auto* sub    = sections[2];
    auto* filter = sections[3];
    auto* unison = sections[4];
    auto* drive  = sections[5];
    auto* ampEnv = sections[6];
    auto* filEnv = sections[7];
    auto* lfo1   = sections[8];
    auto* lfo2   = sections[9];
    auto* modMtx = sections[10];
    auto* global = sections[11];

    std::vector<Row> rows = {
        { { osc1, osc2, sub },          { 1.0f, 1.0f, 1.0f },        1.05f },
        { { filter, unison, drive },    { 1.4f, 1.0f, 1.0f },        1.05f },
        { { ampEnv, filEnv, lfo1, lfo2},{ 1.0f, 1.0f, 1.0f, 1.0f },  0.85f },
        { { modMtx, global },           { 2.3f, 1.0f },              1.8f  },
    };

    float totalWeight = 0.0f;
    for (auto& r : rows) totalWeight += r.height;

    const int gap = 8;
    const int usableH = area.getHeight() - gap * ((int) rows.size() - 1);
    int y = area.getY();

    for (auto& r : rows)
    {
        const int rowH = (int) (usableH * (r.height / totalWeight));
        int x = area.getX();

        float wSum = 0.0f;
        for (float wgt : r.widths) wSum += wgt;
        const int usableW = area.getWidth() - gap * ((int) r.panels.size() - 1);

        for (size_t i = 0; i < r.panels.size(); ++i)
        {
            const int cellW = (int) (usableW * (r.widths[i] / wSum));
            r.panels[i]->setBounds (x, y, cellW, rowH);
            x += cellW + gap;
        }
        y += rowH + gap;
    }
}
