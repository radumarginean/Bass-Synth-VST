#pragma once

#include <cmath>

namespace bassforge
{
enum class FilterMode
{
    lowpass24 = 0,  // 4-pole / 24 dB - the classic bass filter
    lowpass12,      // 2-pole / 12 dB
    highpass24,
    bandpass,
    numModes
};

/**
    Non-linear Moog-style transistor ladder filter.

    Based on Antti Huovilainen's model (the same lineage Surge and Vaporizer2
    draw their ladder emulations from): four one-pole stages with tanh
    saturation in the feedback path, run at 2x oversampling for stability at
    high resonance. Drive pushes the signal harder into the non-linearities for
    the growly, compressed low end bass patches live on.
*/
class LadderFilter
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset() noexcept
    {
        for (auto& s : stage)   s = 0.0;
        for (auto& d : delay)   d = 0.0;
    }

    void setMode (FilterMode m)  noexcept { mode = m; }

    /** @param cutoffHz   filter cutoff in Hz
        @param resonance  0..1 (self-oscillates near 1)
        @param drive      >= 1 pushes harder into saturation
    */
    void setParams (float cutoffHz, float resonance, float drive) noexcept
    {
        const double fc = clampd (cutoffHz, 20.0, sampleRate * 0.45);

        // Tuning for the 2x-oversampled model.
        const double f  = fc / (sampleRate * 2.0);
        const double wc = f * pi2;
        g  = wc / (1.0 + wc);           // one-pole coefficient
        res = clampd (resonance, 0.0, 1.0) * 4.0; // feedback amount
        drv = drive < 1.0f ? 1.0f : drive;
    }

    inline float process (float in) noexcept
    {
        // 2x oversampling: run the ladder twice per sample, then decimate.
        double out = 0.0;
        for (int os = 0; os < 2; ++os)
            out = tick ((double) in * drv);
        return (float) out / drv * makeupGain();
    }

private:
    inline double tick (double x) noexcept
    {
        // Feedback with saturation.
        double fb = res * (delay[3]);
        double input = x - fb;

        // Four cascaded non-linear one-pole stages.
        stage[0] += g * (fastTanh (input)    - fastTanh (stage[0]));
        stage[1] += g * (fastTanh (stage[0]) - fastTanh (stage[1]));
        stage[2] += g * (fastTanh (stage[1]) - fastTanh (stage[2]));
        stage[3] += g * (fastTanh (stage[2]) - fastTanh (stage[3]));

        delay[3] = stage[3];

        switch (mode)
        {
            case FilterMode::lowpass24: return stage[3];
            case FilterMode::lowpass12: return stage[1];
            case FilterMode::highpass24: return x - stage[3];      // input minus LP
            case FilterMode::bandpass:  return stage[1] - stage[3]; // LP12 - LP24
            default:                    return stage[3];
        }
    }

    // Slight makeup so high resonance doesn't gut the level.
    inline float makeupGain() const noexcept
    {
        return 1.0f + (float) res * 0.15f;
    }

    static inline double fastTanh (double x) noexcept
    {
        if (x < -3.0) return -1.0;
        if (x >  3.0) return  1.0;
        const double x2 = x * x;
        return x * (27.0 + x2) / (27.0 + 9.0 * x2);
    }

    static inline double clampd (double v, double lo, double hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    static constexpr double pi2 = 3.141592653589793 * 2.0;

    double     sampleRate = 44100.0;
    double     g   = 0.5;
    double     res = 0.0;
    float      drv = 1.0f;
    FilterMode mode = FilterMode::lowpass24;

    double stage[4] = { 0, 0, 0, 0 };
    double delay[4] = { 0, 0, 0, 0 };
};
} // namespace bassforge
