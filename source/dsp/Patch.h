#pragma once

#include "Oscillator.h"
#include "LadderFilter.h"
#include "LFO.h"
#include "Parameters.h"

namespace bassforge
{
enum class VoiceMode { poly = 0, mono, legato };
enum class DriveType { soft = 0, hard, fold, bitcrush };

struct ModRoute
{
    ModSource src = ModSource::none;
    ModDest   dst = ModDest::none;
    float     amt = 0.0f;
};

/**
    A flat, plain-data snapshot of every parameter for one processing block.

    The processor reads the atomic APVTS parameter pointers once per block and
    fills this struct; voices then read from it with no locking and no atomics
    in the audio-rate inner loop.
*/
struct Patch
{
    // Osc 1
    OscWave osc1Wave = OscWave::saw;
    int     osc1Oct = 0, osc1Semi = 0;
    float   osc1Fine = 0.0f, osc1Level = 0.8f, osc1Pw = 0.5f, osc1Wt = 0.0f;

    // Osc 2
    OscWave osc2Wave = OscWave::saw;
    int     osc2Oct = -1, osc2Semi = 0;
    float   osc2Fine = 0.0f, osc2Level = 0.0f, osc2Pw = 0.5f, osc2Wt = 0.0f;
    bool    osc2Sync = false;

    // Sub + noise
    int   subWave = 0;      // 0 = sine, 1 = square
    int   subOct = -1;
    float subLevel = 0.5f, noiseLevel = 0.0f;

    // Unison
    int   uniVoices = 1;
    float uniDetune = 0.2f, uniWidth = 0.5f;

    // Drive
    float     drive = 0.15f;
    DriveType driveType = DriveType::soft;

    // Filter
    FilterMode filterMode = FilterMode::lowpass24;
    float cutoff = 1200.0f, resonance = 0.25f, filterDrive = 1.4f;
    float keyTrack = 0.35f, filterEnvAmt = 0.6f;

    // Envelopes
    float ampA = 0.005f, ampD = 0.25f, ampS = 0.8f, ampR = 0.15f;
    float fegA = 0.005f, fegD = 0.30f, fegS = 0.15f, fegR = 0.20f;

    // LFOs
    LfoShape lfo1Shape = LfoShape::sine;   float lfo1Rate = 4.0f;  bool lfo1Sync = false;
    LfoShape lfo2Shape = LfoShape::triangle; float lfo2Rate = 0.5f; bool lfo2Sync = false;

    // Global
    VoiceMode voiceMode = VoiceMode::poly;
    float     glide = 0.0f;
    int       bendRange = 2;
    float     masterVol = 0.8f;

    // Mod matrix
    ModRoute routes[numModSlots];

    // Host context (updated per block)
    double tempoBpm = 120.0;
};
} // namespace bassforge
