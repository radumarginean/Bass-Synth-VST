#pragma once

#include <cmath>

namespace bassforge
{
/**
    Analog-style ADSR envelope with exponential segments.

    Exponential attack/decay/release curves (rather than linear ramps) give the
    snappy, punchy transients that make bass plucks and stabs sit in a mix -
    the same curve shaping Surge and Vaporizer2 expose on their envelopes.
*/
class Envelope
{
public:
    enum class Stage { idle, attack, decay, sustain, release };

    void prepare (double newSampleRate) noexcept { sampleRate = newSampleRate; }

    void setParameters (float attackSec, float decaySec, float sustainLevel, float releaseSec) noexcept
    {
        attackRate  = rateFor (attackSec);
        decayRate   = rateFor (decaySec);
        releaseRate = rateFor (releaseSec);
        sustain     = clamp (sustainLevel, 0.0f, 1.0f);
    }

    void noteOn() noexcept
    {
        stage = Stage::attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::idle)
            stage = Stage::release;
    }

    void reset() noexcept
    {
        stage = Stage::idle;
        value = 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::idle; }

    inline float process() noexcept
    {
        switch (stage)
        {
            case Stage::idle:
                value = 0.0f;
                break;

            case Stage::attack:
                // aim slightly above 1 for a natural exponential knee
                value += attackRate * (1.3f - value);
                if (value >= 1.0f) { value = 1.0f; stage = Stage::decay; }
                break;

            case Stage::decay:
                value += decayRate * (sustain - 0.02f - value);
                if (value <= sustain) { value = sustain; stage = Stage::sustain; }
                break;

            case Stage::sustain:
                value = sustain;
                break;

            case Stage::release:
                value += releaseRate * (-0.02f - value);
                if (value <= 0.0f) { value = 0.0f; stage = Stage::idle; }
                break;
        }
        return value;
    }

private:
    // Convert a time in seconds into a per-sample one-pole rate.
    float rateFor (float seconds) const noexcept
    {
        seconds = seconds < 0.0005f ? 0.0005f : seconds;
        return 1.0f - std::exp (-1.0f / ((float) sampleRate * seconds));
    }

    static inline float clamp (float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    double sampleRate  = 44100.0;
    float  value       = 0.0f;
    float  attackRate  = 0.01f;
    float  decayRate   = 0.01f;
    float  releaseRate = 0.01f;
    float  sustain     = 0.8f;
    Stage  stage       = Stage::idle;
};
} // namespace bassforge
