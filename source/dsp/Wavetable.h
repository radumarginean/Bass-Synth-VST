#pragma once

#include <array>
#include <cmath>
#include <vector>

namespace bassforge
{
/**
    A band-limited, multi-frame wavetable.

    This is the shared heart of both Surge XT and Vaporizer2: rather than a
    single static waveform, we store several morphable "frames" and generate a
    pyramid of mip levels per frame so that playback stays alias-free across the
    whole keyboard range. Higher notes read from mip levels that contain fewer
    harmonics.

    Frames morph timbre from a pure sine (frame 0) through triangle, sawtooth
    and finally a hollow square-ish spectrum (last frame). Sweeping the frame
    position gives the classic wavetable movement.
*/
class Wavetable
{
public:
    static constexpr int  tableSize = 2048;         // samples per single cycle
    static constexpr int  numFrames = 8;            // morph frames
    static constexpr int  numMips   = 11;           // mip levels (>= log2(tableSize))

    Wavetable() { build(); }

    /** Returns an interpolated sample.
        @param frameNorm   morph position in [0, 1]
        @param mipLevel    selected mip level (0 = full bandwidth)
        @param phase       read phase in [0, 1)
    */
    inline float read (float frameNorm, int mipLevel, float phase) const noexcept
    {
        frameNorm = juce_clamp (frameNorm, 0.0f, 1.0f);
        mipLevel  = mipLevel < 0 ? 0 : (mipLevel >= numMips ? numMips - 1 : mipLevel);

        const float framePos = frameNorm * (numFrames - 1);
        const int   f0       = (int) framePos;
        const int   f1       = f0 < numFrames - 1 ? f0 + 1 : f0;
        const float fFrac    = framePos - (float) f0;

        const float s0 = readFrame (f0, mipLevel, phase);
        const float s1 = readFrame (f1, mipLevel, phase);
        return s0 + (s1 - s0) * fFrac;
    }

    /** Chooses the mip level that keeps the top harmonic below Nyquist. */
    inline int mipForFrequency (double frequency, double sampleRate) const noexcept
    {
        if (frequency <= 0.0)
            return numMips - 1;

        const double nyquist    = sampleRate * 0.5;
        const double maxHarm    = nyquist / frequency;   // harmonics we may keep
        int level = 0;
        int harmonicsAtLevel = tableSize / 2;            // full-bandwidth harmonic count
        while (level < numMips - 1 && harmonicsAtLevel > maxHarm)
        {
            harmonicsAtLevel >>= 1;
            ++level;
        }
        return level;
    }

private:
    // frames[frame][mip] -> table of (tableSize + 1) samples (guard point at end)
    std::array<std::array<std::vector<float>, numMips>, numFrames> frames;

    static inline float juce_clamp (float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline float readFrame (int frame, int mip, float phase) const noexcept
    {
        const auto& table = frames[(size_t) frame][(size_t) mip];
        const float pos   = phase * tableSize;
        const int   i0    = (int) pos;
        const float frac  = pos - (float) i0;
        const float a     = table[(size_t) i0];
        const float b     = table[(size_t) (i0 + 1)];
        return a + (b - a) * frac;
    }

    // Harmonic amplitude for a given frame's target spectrum.
    static double harmonicAmplitude (int frame, int harmonic)
    {
        const double n = (double) harmonic;

        // Four archetype spectra we crossfade between across the frame range.
        const double sine     = (harmonic == 1) ? 1.0 : 0.0;
        const double triangle = (harmonic % 2 == 1)
                                    ? (1.0 / (n * n)) * ((((harmonic - 1) / 2) % 2) ? -1.0 : 1.0)
                                    : 0.0;
        const double saw      = 1.0 / n;
        const double square   = (harmonic % 2 == 1) ? (1.0 / n) : 0.0;

        const double t = (double) frame / (double) (numFrames - 1); // 0..1
        // Piecewise crossfade: sine -> triangle -> saw -> square
        const double seg = t * 3.0;
        if (seg < 1.0)      { const double x = seg;        return sine     * (1.0 - x) + triangle * x; }
        else if (seg < 2.0) { const double x = seg - 1.0;  return triangle * (1.0 - x) + saw      * x; }
        else                { const double x = seg - 2.0;  return saw      * (1.0 - x) + square   * x; }
    }

    void build()
    {
        const double twoPi = 6.283185307179586476925;

        for (int frame = 0; frame < numFrames; ++frame)
        {
            for (int mip = 0; mip < numMips; ++mip)
            {
                const int maxHarmonics = juce_max (1, (tableSize / 2) >> mip);

                auto& table = frames[(size_t) frame][(size_t) mip];
                table.assign ((size_t) tableSize + 1, 0.0f);

                double peak = 0.0;
                for (int i = 0; i < tableSize; ++i)
                {
                    const double ph = twoPi * (double) i / (double) tableSize;
                    double acc = 0.0;
                    for (int h = 1; h <= maxHarmonics; ++h)
                        acc += harmonicAmplitude (frame, h) * std::sin (ph * h);

                    table[(size_t) i] = (float) acc;
                    peak = juce_maxd (peak, std::abs (acc));
                }

                // Normalise each mip so morphing keeps roughly constant level.
                if (peak > 1.0e-9)
                {
                    const float g = (float) (1.0 / peak);
                    for (int i = 0; i < tableSize; ++i)
                        table[(size_t) i] *= g;
                }
                table[(size_t) tableSize] = table[0]; // guard point for interpolation
            }
        }
    }

    static inline int    juce_max  (int a, int b)       noexcept { return a > b ? a : b; }
    static inline double juce_maxd (double a, double b) noexcept { return a > b ? a : b; }
};
} // namespace bassforge
