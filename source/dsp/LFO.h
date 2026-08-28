#pragma once

#include <cmath>
#include <cstdint>

namespace bassforge
{
enum class LfoShape
{
    sine = 0,
    triangle,
    saw,
    square,
    sampleAndHold,
    numShapes
};

/**
    Low-frequency modulation source.

    Free-running or tempo-synced. Output is bipolar in [-1, 1]. The sample &
    hold shape uses a simple xorshift PRNG so it stays cheap and deterministic
    per voice.
*/
class LFO
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        phase = 0.0;
    }

    void reset (double startPhase = 0.0) noexcept
    {
        phase   = startPhase - std::floor (startPhase);
        current = 0.0f;
        held    = 0.0f;
        lastPhase = phase;
    }

    void setShape (LfoShape s) noexcept { shape = s; }
    void setRate (float hz)    noexcept { rateHz = hz < 0.0f ? 0.0f : hz; }

    inline float process() noexcept
    {
        const double inc = rateHz / sampleRate;

        switch (shape)
        {
            case LfoShape::sine:     current = (float) std::sin (phase * twoPi); break;
            case LfoShape::triangle: current = (float) (4.0 * std::abs (phase - 0.5) - 1.0); break;
            case LfoShape::saw:      current = (float) (2.0 * phase - 1.0); break;
            case LfoShape::square:   current = phase < 0.5 ? 1.0f : -1.0f; break;
            case LfoShape::sampleAndHold:
                if (phase < lastPhase) held = nextRandom();
                current = held;
                break;
            default: break;
        }

        lastPhase = phase;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        return current;
    }

private:
    inline float nextRandom() noexcept
    {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return ((float) (rng & 0xffffff) / (float) 0x800000) - 1.0f;
    }

    static constexpr double twoPi = 6.283185307179586476925;

    double   sampleRate = 44100.0;
    double   phase      = 0.0;
    double   lastPhase  = 0.0;
    float    rateHz     = 1.0f;
    float    current    = 0.0f;
    float    held       = 0.0f;
    uint32_t rng        = 0x1234567u;
    LfoShape shape      = LfoShape::sine;
};
} // namespace bassforge
