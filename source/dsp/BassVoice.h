#pragma once

#include "Patch.h"
#include "Oscillator.h"
#include "LadderFilter.h"
#include "Envelope.h"
#include "LFO.h"
#include "Wavetable.h"

namespace bassforge
{
/**
    A single polyphonic voice.

    Contains two unison oscillator banks, a sub oscillator, a noise source,
    stereo ladder filters, amp/filter envelopes and two LFOs. The modulation
    matrix is evaluated per sample so LFOs and envelopes can drive pitch,
    cutoff, level, pan and more.
*/
class BassVoice
{
public:
    static constexpr int maxUnison = 7;

    void prepare (double sampleRate, const Wavetable* sharedTable);

    void startNote (int midiNote, float velocity, bool doGlide, double glideFromNote);
    void stopNote();
    void kill();                       // immediate fade for voice stealing

    bool isActive() const noexcept { return active; }
    int  getNote()  const noexcept { return currentMidiNote; }
    bool isReleasing() const noexcept { return releasing; }
    juce::uint32 getStartOrder() const noexcept { return startOrder; }

    /** Adds this voice's stereo output into the supplied buffers. */
    void render (float* left, float* right, int numSamples,
                 const Patch& patch, float modWheel, float pitchBendSemis);

    void setStartOrder (juce::uint32 order) noexcept { startOrder = order; }

private:
    static inline double noteToFreq (double note) noexcept
    {
        return 440.0 * std::pow (2.0, (note - 69.0) / 12.0);
    }

    float applyDrive (float x, float amount, DriveType type) const noexcept;
    void  updateUnison (const Patch& patch) noexcept;

    double sampleRate = 44100.0;
    const Wavetable* wavetable = nullptr;

    Oscillator osc1[maxUnison];
    Oscillator osc2[maxUnison];
    Oscillator subOsc;

    LadderFilter filterL, filterR;
    Envelope     ampEnv, filtEnv;
    LFO          lfo1, lfo2;

    // Unison layout, refreshed each block
    int   uniCount = 1;
    float uniRatio[maxUnison] { 1.0f };
    float uniGainL[maxUnison] { 1.0f };
    float uniGainR[maxUnison] { 1.0f };

    // Pitch / glide
    double currentNote     = 60.0;
    double targetNote      = 60.0;
    double glideCoeff      = 1.0;
    int    currentMidiNote = -1;

    float  velocity  = 1.0f;
    bool   active    = false;
    bool   releasing = false;
    juce::uint32 startOrder = 0;

    juce::Random noiseRng;
};
} // namespace bassforge
