#pragma once

#include <JuceHeader.h>
#include "Patch.h"
#include "Parameters.h"
#include <cmath>

namespace bassforge
{
/** Snaps an LFO rate knob to the nearest musical division when tempo sync is on. */
inline float resolveLfoRate (juce::AudioProcessorValueTreeState& apvts,
                             const char* rateId, const char* syncId, double bpm)
{
    const float rawHz = apvts.getRawParameterValue (rateId)->load();
    const bool  sync  = apvts.getRawParameterValue (syncId)->load() > 0.5f;
    if (! sync || bpm <= 0.0)
        return rawHz;

    static const double beatsPerCycle[] = { 8, 4, 3, 2, 1.5, 1, 0.75, 0.5, 0.375, 0.25, 0.125 };
    const double hzPerBeat = bpm / 60.0;

    float bestHz = rawHz;
    double bestErr = 1.0e9;
    for (double b : beatsPerCycle)
    {
        const double hz = hzPerBeat / b;
        const double err = std::abs (hz - rawHz);
        if (err < bestErr) { bestErr = err; bestHz = (float) hz; }
    }
    return bestHz;
}

/** Reads the APVTS parameter values into a flat, audio-thread-friendly Patch. */
inline void buildPatch (juce::AudioProcessorValueTreeState& apvts, Patch& p, double bpm)
{
    auto val  = [&apvts] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto ival = [&val]   (const char* id) { return (int) std::lround (val (id)); };

    p.osc1Wave  = (OscWave) ival (pid::osc1Wave);
    p.osc1Oct   = ival (pid::osc1Oct);
    p.osc1Semi  = ival (pid::osc1Semi);
    p.osc1Fine  = val (pid::osc1Fine);
    p.osc1Level = val (pid::osc1Level);
    p.osc1Pw    = val (pid::osc1Pw);
    p.osc1Wt    = val (pid::osc1Wt);

    p.osc2Wave  = (OscWave) ival (pid::osc2Wave);
    p.osc2Oct   = ival (pid::osc2Oct);
    p.osc2Semi  = ival (pid::osc2Semi);
    p.osc2Fine  = val (pid::osc2Fine);
    p.osc2Level = val (pid::osc2Level);
    p.osc2Pw    = val (pid::osc2Pw);
    p.osc2Wt    = val (pid::osc2Wt);
    p.osc2Sync  = val (pid::osc2Sync) > 0.5f;

    p.subWave    = ival (pid::subWave);
    p.subOct     = ival (pid::subOct);
    p.subLevel   = val (pid::subLevel);
    p.noiseLevel = val (pid::noiseLevel);

    p.uniVoices = ival (pid::uniVoices);
    p.uniDetune = val (pid::uniDetune);
    p.uniWidth  = val (pid::uniWidth);

    p.drive     = val (pid::drive);
    p.driveType = (DriveType) ival (pid::driveType);

    p.filterMode   = (FilterMode) ival (pid::filterMode);
    p.cutoff       = val (pid::cutoff);
    p.resonance    = val (pid::resonance);
    p.filterDrive  = val (pid::filterDrv);
    p.keyTrack     = val (pid::keyTrack);
    p.filterEnvAmt = val (pid::filterEnvAmt);

    p.ampA = val (pid::ampA); p.ampD = val (pid::ampD); p.ampS = val (pid::ampS); p.ampR = val (pid::ampR);
    p.fegA = val (pid::fegA); p.fegD = val (pid::fegD); p.fegS = val (pid::fegS); p.fegR = val (pid::fegR);

    p.lfo1Shape = (LfoShape) ival (pid::lfo1Shape);
    p.lfo1Rate  = resolveLfoRate (apvts, pid::lfo1Rate, pid::lfo1Sync, bpm);
    p.lfo1Sync  = val (pid::lfo1Sync) > 0.5f;
    p.lfo2Shape = (LfoShape) ival (pid::lfo2Shape);
    p.lfo2Rate  = resolveLfoRate (apvts, pid::lfo2Rate, pid::lfo2Sync, bpm);
    p.lfo2Sync  = val (pid::lfo2Sync) > 0.5f;

    p.voiceMode = (VoiceMode) ival (pid::voiceMode);
    p.glide     = val (pid::glide);
    p.bendRange = ival (pid::bendRange);
    p.masterVol = val (pid::masterVol);

    for (int s = 0; s < numModSlots; ++s)
    {
        p.routes[s].src = (ModSource) ival (pid::modSrc (s).toRawUTF8());
        p.routes[s].dst = (ModDest)   ival (pid::modDst (s).toRawUTF8());
        p.routes[s].amt = val (pid::modAmt (s).toRawUTF8());
    }

    p.tempoBpm = bpm;
}
} // namespace bassforge
