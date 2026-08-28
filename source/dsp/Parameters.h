#pragma once

#include <JuceHeader.h>

namespace bassforge
{
// ---------------------------------------------------------------------------
// Modulation matrix vocabulary (a compact take on Surge XT's mod matrix)
// ---------------------------------------------------------------------------
enum class ModSource
{
    none = 0,
    lfo1,
    lfo2,
    filterEnv,
    ampEnv,
    velocity,
    modWheel,
    keyTrack,
    numSources
};

enum class ModDest
{
    none = 0,
    osc1Pitch,
    osc2Pitch,
    cutoff,
    resonance,
    osc1Level,
    osc2Level,
    pulseWidth,
    wavetablePos,
    amplitude,
    pan,
    numDests
};

inline juce::StringArray modSourceNames()
{
    return { "None", "LFO 1", "LFO 2", "Filter Env", "Amp Env",
             "Velocity", "Mod Wheel", "Key Track" };
}

inline juce::StringArray modDestNames()
{
    return { "None", "Osc1 Pitch", "Osc2 Pitch", "Cutoff", "Resonance",
             "Osc1 Level", "Osc2 Level", "Pulse Width", "WT Position",
             "Amplitude", "Pan" };
}

static constexpr int numModSlots = 6;

// ---------------------------------------------------------------------------
// Parameter IDs
// ---------------------------------------------------------------------------
namespace pid
{
    // Oscillator 1
    inline constexpr const char* osc1Wave   = "osc1_wave";
    inline constexpr const char* osc1Oct    = "osc1_oct";
    inline constexpr const char* osc1Semi   = "osc1_semi";
    inline constexpr const char* osc1Fine   = "osc1_fine";
    inline constexpr const char* osc1Level  = "osc1_level";
    inline constexpr const char* osc1Pw     = "osc1_pw";
    inline constexpr const char* osc1Wt     = "osc1_wt";

    // Oscillator 2
    inline constexpr const char* osc2Wave   = "osc2_wave";
    inline constexpr const char* osc2Oct    = "osc2_oct";
    inline constexpr const char* osc2Semi   = "osc2_semi";
    inline constexpr const char* osc2Fine   = "osc2_fine";
    inline constexpr const char* osc2Level  = "osc2_level";
    inline constexpr const char* osc2Pw     = "osc2_pw";
    inline constexpr const char* osc2Wt     = "osc2_wt";
    inline constexpr const char* osc2Sync   = "osc2_sync";

    // Sub oscillator + noise
    inline constexpr const char* subWave    = "sub_wave";
    inline constexpr const char* subOct     = "sub_oct";
    inline constexpr const char* subLevel   = "sub_level";
    inline constexpr const char* noiseLevel = "noise_level";

    // Unison
    inline constexpr const char* uniVoices  = "uni_voices";
    inline constexpr const char* uniDetune  = "uni_detune";
    inline constexpr const char* uniWidth   = "uni_width";

    // Drive (pre-filter)
    inline constexpr const char* drive      = "drive";
    inline constexpr const char* driveType  = "drive_type";

    // Filter
    inline constexpr const char* filterMode = "filter_mode";
    inline constexpr const char* cutoff     = "cutoff";
    inline constexpr const char* resonance  = "resonance";
    inline constexpr const char* filterDrv  = "filter_drive";
    inline constexpr const char* keyTrack   = "filter_keytrack";
    inline constexpr const char* filterEnvAmt = "filter_env_amt";

    // Amp envelope
    inline constexpr const char* ampA = "amp_a";
    inline constexpr const char* ampD = "amp_d";
    inline constexpr const char* ampS = "amp_s";
    inline constexpr const char* ampR = "amp_r";

    // Filter envelope
    inline constexpr const char* fegA = "feg_a";
    inline constexpr const char* fegD = "feg_d";
    inline constexpr const char* fegS = "feg_s";
    inline constexpr const char* fegR = "feg_r";

    // LFOs
    inline constexpr const char* lfo1Shape = "lfo1_shape";
    inline constexpr const char* lfo1Rate  = "lfo1_rate";
    inline constexpr const char* lfo1Sync  = "lfo1_sync";
    inline constexpr const char* lfo2Shape = "lfo2_shape";
    inline constexpr const char* lfo2Rate  = "lfo2_rate";
    inline constexpr const char* lfo2Sync  = "lfo2_sync";

    // Global / voicing
    inline constexpr const char* voiceMode = "voice_mode"; // poly / mono / legato
    inline constexpr const char* glide     = "glide";
    inline constexpr const char* bendRange = "bend_range";
    inline constexpr const char* masterVol = "master_vol";

    // Mod matrix slot IDs are built dynamically:  mod{n}_src / mod{n}_dst / mod{n}_amt
    inline juce::String modSrc (int slot) { return "mod" + juce::String (slot) + "_src"; }
    inline juce::String modDst (int slot) { return "mod" + juce::String (slot) + "_dst"; }
    inline juce::String modAmt (int slot) { return "mod" + juce::String (slot) + "_amt"; }
}

// ---------------------------------------------------------------------------
// Parameter layout
// ---------------------------------------------------------------------------
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using APF   = juce::AudioParameterFloat;
    using API   = juce::AudioParameterInt;
    using APC   = juce::AudioParameterChoice;
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const juce::StringArray waveNames { "Saw", "Square", "Triangle", "Sine", "Wavetable" };
    const juce::StringArray subWaveNames { "Sine", "Square" };
    const juce::StringArray filterModeNames { "LP 24dB", "LP 12dB", "HP 24dB", "Bandpass" };
    const juce::StringArray lfoShapeNames { "Sine", "Triangle", "Saw", "Square", "S&H" };
    const juce::StringArray driveTypeNames { "Soft", "Hard", "Fold", "Bitcrush" };
    const juce::StringArray voiceModeNames { "Poly", "Mono", "Legato" };
    const juce::StringArray syncRateNames { "8/1","4/1","2/1","1/1","1/2","1/4","1/8","1/16","1/32" };

    auto pct  = [] { return Range (0.0f, 1.0f, 0.001f); };

    // Cutoff uses a musical (skewed) frequency range.
    Range cutoffRange (20.0f, 18000.0f, 0.0f, 0.25f);

    // ---- OSC 1 ----
    params.push_back (std::make_unique<APC> (pid::osc1Wave, "Osc1 Wave", waveNames, 0));
    params.push_back (std::make_unique<API> (pid::osc1Oct,  "Osc1 Octave", -3, 2, 0));
    params.push_back (std::make_unique<API> (pid::osc1Semi, "Osc1 Semi", -12, 12, 0));
    params.push_back (std::make_unique<APF>(pid::osc1Fine, "Osc1 Fine", Range (-100.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<APF>(pid::osc1Level,"Osc1 Level", pct(), 0.8f));
    params.push_back (std::make_unique<APF>(pid::osc1Pw,   "Osc1 PW", Range (0.02f, 0.98f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF>(pid::osc1Wt,   "Osc1 WT Pos", pct(), 0.0f));

    // ---- OSC 2 ----
    params.push_back (std::make_unique<APC> (pid::osc2Wave, "Osc2 Wave", waveNames, 0));
    params.push_back (std::make_unique<API> (pid::osc2Oct,  "Osc2 Octave", -3, 2, -1));
    params.push_back (std::make_unique<API> (pid::osc2Semi, "Osc2 Semi", -12, 12, 0));
    params.push_back (std::make_unique<APF>(pid::osc2Fine, "Osc2 Fine", Range (-100.0f, 100.0f, 0.1f), 7.0f));
    params.push_back (std::make_unique<APF>(pid::osc2Level,"Osc2 Level", pct(), 0.0f));
    params.push_back (std::make_unique<APF>(pid::osc2Pw,   "Osc2 PW", Range (0.02f, 0.98f, 0.001f), 0.5f));
    params.push_back (std::make_unique<APF>(pid::osc2Wt,   "Osc2 WT Pos", pct(), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(pid::osc2Sync, "Osc2 Sync", false));

    // ---- SUB + NOISE ----
    params.push_back (std::make_unique<APC> (pid::subWave, "Sub Wave", subWaveNames, 0));
    params.push_back (std::make_unique<API> (pid::subOct,  "Sub Octave", -2, -1, -1));
    params.push_back (std::make_unique<APF>(pid::subLevel, "Sub Level", pct(), 0.5f));
    params.push_back (std::make_unique<APF>(pid::noiseLevel, "Noise Level", pct(), 0.0f));

    // ---- UNISON ----
    params.push_back (std::make_unique<API> (pid::uniVoices, "Unison Voices", 1, 7, 1));
    params.push_back (std::make_unique<APF>(pid::uniDetune, "Unison Detune", pct(), 0.2f));
    params.push_back (std::make_unique<APF>(pid::uniWidth,  "Unison Width", pct(), 0.5f));

    // ---- DRIVE ----
    params.push_back (std::make_unique<APF>(pid::drive, "Drive", Range (0.0f, 1.0f, 0.001f), 0.15f));
    params.push_back (std::make_unique<APC> (pid::driveType, "Drive Type", driveTypeNames, 0));

    // ---- FILTER ----
    params.push_back (std::make_unique<APC> (pid::filterMode, "Filter Mode", filterModeNames, 0));
    params.push_back (std::make_unique<APF>(pid::cutoff, "Cutoff", cutoffRange, 1200.0f));
    params.push_back (std::make_unique<APF>(pid::resonance, "Resonance", pct(), 0.25f));
    params.push_back (std::make_unique<APF>(pid::filterDrv, "Filter Drive", Range (1.0f, 6.0f, 0.01f), 1.4f));
    params.push_back (std::make_unique<APF>(pid::keyTrack, "Key Track", pct(), 0.35f));
    params.push_back (std::make_unique<APF>(pid::filterEnvAmt, "Filter Env Amt", Range (-1.0f, 1.0f, 0.001f), 0.6f));

    // ---- AMP ENV ----
    Range timeR (0.0005f, 8.0f, 0.0f, 0.3f);
    params.push_back (std::make_unique<APF>(pid::ampA, "Amp Attack", timeR, 0.005f));
    params.push_back (std::make_unique<APF>(pid::ampD, "Amp Decay", timeR, 0.25f));
    params.push_back (std::make_unique<APF>(pid::ampS, "Amp Sustain", pct(), 0.8f));
    params.push_back (std::make_unique<APF>(pid::ampR, "Amp Release", timeR, 0.15f));

    // ---- FILTER ENV ----
    params.push_back (std::make_unique<APF>(pid::fegA, "Filt Attack", timeR, 0.005f));
    params.push_back (std::make_unique<APF>(pid::fegD, "Filt Decay", timeR, 0.30f));
    params.push_back (std::make_unique<APF>(pid::fegS, "Filt Sustain", pct(), 0.15f));
    params.push_back (std::make_unique<APF>(pid::fegR, "Filt Release", timeR, 0.20f));

    // ---- LFOs ----
    Range lfoRateR (0.01f, 40.0f, 0.0f, 0.35f);
    params.push_back (std::make_unique<APC> (pid::lfo1Shape, "LFO1 Shape", lfoShapeNames, 0));
    params.push_back (std::make_unique<APF>(pid::lfo1Rate, "LFO1 Rate", lfoRateR, 4.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(pid::lfo1Sync, "LFO1 Tempo Sync", false));
    params.push_back (std::make_unique<APC> (pid::lfo2Shape, "LFO2 Shape", lfoShapeNames, 1));
    params.push_back (std::make_unique<APF>(pid::lfo2Rate, "LFO2 Rate", lfoRateR, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterBool>(pid::lfo2Sync, "LFO2 Tempo Sync", false));

    // ---- GLOBAL ----
    params.push_back (std::make_unique<APC> (pid::voiceMode, "Voice Mode", voiceModeNames, 0));
    params.push_back (std::make_unique<APF>(pid::glide, "Glide", Range (0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back (std::make_unique<API> (pid::bendRange, "Bend Range", 1, 24, 2));
    params.push_back (std::make_unique<APF>(pid::masterVol, "Master Volume", Range (0.0f, 1.5f, 0.001f), 0.8f));

    // ---- MOD MATRIX ----
    const auto srcNames = modSourceNames();
    const auto dstNames = modDestNames();
    for (int slot = 0; slot < numModSlots; ++slot)
    {
        const int defSrc = (slot == 0) ? (int) ModSource::lfo1
                         : (slot == 1) ? (int) ModSource::modWheel
                         : 0;
        const int defDst = (slot == 0) ? (int) ModDest::cutoff
                         : (slot == 1) ? (int) ModDest::cutoff
                         : 0;
        params.push_back (std::make_unique<APC> (pid::modSrc (slot), "Mod" + juce::String (slot + 1) + " Src", srcNames, defSrc));
        params.push_back (std::make_unique<APC> (pid::modDst (slot), "Mod" + juce::String (slot + 1) + " Dst", dstNames, defDst));
        params.push_back (std::make_unique<APF>(pid::modAmt (slot), "Mod" + juce::String (slot + 1) + " Amt", Range (-1.0f, 1.0f, 0.001f), 0.0f));
    }

    return { params.begin(), params.end() };
}
} // namespace bassforge
