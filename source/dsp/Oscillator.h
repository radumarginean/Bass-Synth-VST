#pragma once

#include "Wavetable.h"
#include <cmath>

namespace bassforge
{
enum class OscWave
{
    saw = 0,
    square,
    triangle,
    sine,
    wavetable,
    numWaves
};

/**
    A single anti-aliased oscillator.

    Classic shapes (saw / square / triangle) use PolyBLEP correction for cheap,
    alias-suppressed edges - the technique Surge uses for its "Classic"
    oscillator. The wavetable mode reads the band-limited multi-frame table
    (the Vaporizer2 / Surge "Wavetable" flavour) with per-note mip selection.

    A shared Wavetable instance is injected so the (fairly heavy) table build
    happens once for the whole synth rather than per voice.
*/
class Oscillator
{
public:
    void prepare (double newSampleRate, const Wavetable* sharedTable) noexcept
    {
        sampleRate = newSampleRate;
        table      = sharedTable;
        phase      = 0.0;
    }

    void reset() noexcept { phase = 0.0; }

    void setWave (OscWave w)          noexcept { wave = w; }
    void setPulseWidth (float pw)     noexcept { pulseWidth = clamp (pw, 0.02f, 0.98f); }
    void setWavetablePos (float p)    noexcept { wtPos = clamp (p, 0.0f, 1.0f); }

    /** Randomises the start phase (used for unison so voices don't stack). */
    void setPhase (double p) noexcept { phase = p - std::floor (p); }

    /** True if the previous process() call wrapped the phase (for hard sync). */
    bool wrapped() const noexcept { return didWrap; }

    /** Forces the phase back to zero (slave reset in hard-sync mode). */
    void hardSync() noexcept { phase = 0.0; triState = 0.0f; }

    inline float process (double frequency) noexcept
    {
        if (frequency < 0.0) frequency = 0.0;
        const double inc = frequency / sampleRate;

        float out = 0.0f;
        switch (wave)
        {
            case OscWave::saw:       out = renderSaw (inc);            break;
            case OscWave::square:    out = renderSquare (inc);         break;
            case OscWave::triangle:  out = renderTriangle (inc);       break;
            case OscWave::sine:      out = (float) std::sin (phase * twoPi); break;
            case OscWave::wavetable: out = renderWavetable (frequency); break;
            default:                 break;
        }

        phase += inc;
        didWrap = phase >= 1.0;
        if (didWrap) phase -= 1.0;
        return out;
    }

private:
    // ---- PolyBLEP helpers -------------------------------------------------
    static inline double polyBlep (double t, double dt) noexcept
    {
        if (t < dt)            { t /= dt;        return t + t - t * t - 1.0; }
        else if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    inline float renderSaw (double dt) noexcept
    {
        double v = 2.0 * phase - 1.0;
        v -= polyBlep (phase, dt);
        return (float) v;
    }

    inline float renderSquare (double dt) noexcept
    {
        double v = phase < pulseWidth ? 1.0 : -1.0;
        v += polyBlep (phase, dt);
        double t2 = phase + (1.0 - pulseWidth);
        if (t2 >= 1.0) t2 -= 1.0;
        v -= polyBlep (t2, dt);
        return (float) v;
    }

    inline float renderTriangle (double dt) noexcept
    {
        // Integrate a PolyBLEP square to get an alias-suppressed triangle.
        double sq = phase < 0.5 ? 1.0 : -1.0;
        sq += polyBlep (phase, dt);
        double t2 = phase + 0.5;
        if (t2 >= 1.0) t2 -= 1.0;
        sq -= polyBlep (t2, dt);

        triState += (float) (4.0 * dt * sq); // leaky integrator
        triState *= 0.999f;
        return triState;
    }

    inline float renderWavetable (double frequency) noexcept
    {
        if (table == nullptr) return 0.0f;
        const int mip = table->mipForFrequency (frequency, sampleRate);
        return table->read (wtPos, mip, (float) phase);
    }

    static inline float clamp (float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    static constexpr double twoPi = 6.283185307179586476925;

    double          sampleRate = 44100.0;
    double          phase      = 0.0;
    bool            didWrap    = false;
    float           triState   = 0.0f;
    OscWave         wave       = OscWave::saw;
    float           pulseWidth = 0.5f;
    float           wtPos      = 0.0f;
    const Wavetable* table     = nullptr;
};
} // namespace bassforge
