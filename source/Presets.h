#pragma once

#include <JuceHeader.h>
#include "dsp/Parameters.h"
#include <vector>
#include <utility>
#include <cstdint>
#include <initializer_list>

namespace bassforge
{
/** A named factory patch: a list of (parameter id, real value) overrides. */
struct Preset
{
    juce::String name;
    std::vector<std::pair<juce::String, float>> values;
};

/**
    Applies factory presets to an APVTS.

    Each preset lists only the parameters it cares about; everything else is
    reset to its default first, so presets are self-contained regardless of the
    order the user auditions them in.
*/
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state)
        : apvts (state), presets (buildFactoryPresets()) {}

    int getNumPresets() const { return (int) presets.size(); }
    const std::vector<Preset>& getPresets() const { return presets; }

    juce::StringArray getPresetNames() const
    {
        juce::StringArray names;
        for (auto& p : presets) names.add (p.name);
        return names;
    }

    int getCurrentIndex() const { return currentIndex; }

    void loadPreset (int index)
    {
        if (index < 0 || index >= (int) presets.size())
            return;

        currentIndex = index;

        // Reset everything to defaults so partial presets are complete.
        for (auto* param : apvts.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                ranged->setValueNotifyingHost (ranged->getDefaultValue());

        // Apply this preset's overrides (real values -> normalised).
        for (auto& kv : presets[(size_t) index].values)
            if (auto* p = apvts.getParameter (kv.first))
                p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
    }

private:
    using V = std::vector<std::pair<juce::String, float>>;

    static std::vector<Preset> buildFactoryPresets()
    {
        std::vector<Preset> list;

        // 0 - Init: defaults only.
        list.push_back ({ "Init", {} });

        // 1 - Deep Sub: pure, round, low.
        list.push_back ({ "Deep Sub", V {
            { pid::osc1Wave, 3 }, { pid::osc1Level, 0.7f }, { pid::osc1Oct, 0 },
            { pid::osc2Level, 0.0f },
            { pid::subWave, 0 }, { pid::subOct, -1 }, { pid::subLevel, 0.85f },
            { pid::cutoff, 500.0f }, { pid::resonance, 0.1f }, { pid::filterDrv, 1.6f },
            { pid::filterEnvAmt, 0.2f }, { pid::keyTrack, 0.3f },
            { pid::ampA, 0.006f }, { pid::ampD, 0.6f }, { pid::ampS, 0.9f }, { pid::ampR, 0.25f },
            { pid::voiceMode, 1 }, { pid::glide, 0.05f }, { pid::masterVol, 0.95f },
        }});

        // 2 - Reese Bass: detuned saws, unison, LFO wobble on cutoff.
        list.push_back ({ "Reese Bass", V {
            { pid::osc1Wave, 0 }, { pid::osc1Level, 0.8f },
            { pid::osc2Wave, 0 }, { pid::osc2Level, 0.8f }, { pid::osc2Fine, -9.0f }, { pid::osc2Oct, 0 },
            { pid::subLevel, 0.5f },
            { pid::uniVoices, 5 }, { pid::uniDetune, 0.45f }, { pid::uniWidth, 0.8f },
            { pid::drive, 0.3f }, { pid::driveType, 0 },
            { pid::filterMode, 0 }, { pid::cutoff, 900.0f }, { pid::resonance, 0.35f }, { pid::filterDrv, 2.2f },
            { pid::filterEnvAmt, 0.35f }, { pid::keyTrack, 0.4f },
            { pid::ampA, 0.01f }, { pid::ampD, 0.5f }, { pid::ampS, 0.85f }, { pid::ampR, 0.2f },
            { pid::lfo1Shape, 0 }, { pid::lfo1Rate, 1.5f },
            { pid::modSrc (0), (float) (int) ModSource::lfo1 }, { pid::modDst (0), (float) (int) ModDest::cutoff }, { pid::modAmt (0), 0.4f },
            { pid::masterVol, 0.9f },
        }});

        // 3 - Acid 303: mono saw, screaming resonance, filter env, glide.
        list.push_back ({ "Acid 303", V {
            { pid::osc1Wave, 0 }, { pid::osc1Level, 0.9f },
            { pid::osc2Level, 0.0f }, { pid::subLevel, 0.25f },
            { pid::drive, 0.45f }, { pid::driveType, 0 },
            { pid::filterMode, 0 }, { pid::cutoff, 300.0f }, { pid::resonance, 0.85f }, { pid::filterDrv, 3.0f },
            { pid::filterEnvAmt, 0.8f }, { pid::keyTrack, 0.5f },
            { pid::ampA, 0.003f }, { pid::ampD, 0.4f }, { pid::ampS, 0.6f }, { pid::ampR, 0.1f },
            { pid::fegA, 0.002f }, { pid::fegD, 0.22f }, { pid::fegS, 0.05f }, { pid::fegR, 0.12f },
            { pid::voiceMode, 2 }, { pid::glide, 0.12f }, { pid::masterVol, 0.9f },
        }});

        // 4 - Growl Wobble: LFO2 tempo-synced cutoff sweep + fold drive.
        list.push_back ({ "Growl Wobble", V {
            { pid::osc1Wave, 0 }, { pid::osc1Level, 0.75f },
            { pid::osc2Wave, 1 }, { pid::osc2Level, 0.55f }, { pid::osc2Fine, 5.0f },
            { pid::subLevel, 0.6f },
            { pid::uniVoices, 3 }, { pid::uniDetune, 0.25f },
            { pid::drive, 0.55f }, { pid::driveType, 2 },
            { pid::filterMode, 0 }, { pid::cutoff, 500.0f }, { pid::resonance, 0.6f }, { pid::filterDrv, 2.6f },
            { pid::filterEnvAmt, 0.3f }, { pid::keyTrack, 0.3f },
            { pid::lfo2Shape, 1 }, { pid::lfo2Rate, 2.0f }, { pid::lfo2Sync, 1 },
            { pid::modSrc (0), (float) (int) ModSource::lfo2 }, { pid::modDst (0), (float) (int) ModDest::cutoff }, { pid::modAmt (0), 0.7f },
            { pid::voiceMode, 1 }, { pid::glide, 0.04f }, { pid::masterVol, 0.9f },
        }});

        // 5 - Pluck Bass: snappy filter env, short body.
        list.push_back ({ "Pluck Bass", V {
            { pid::osc1Wave, 0 }, { pid::osc1Level, 0.8f },
            { pid::osc2Wave, 1 }, { pid::osc2Level, 0.35f }, { pid::osc2Oct, 0 }, { pid::osc2Fine, 3.0f },
            { pid::subLevel, 0.55f },
            { pid::filterMode, 0 }, { pid::cutoff, 400.0f }, { pid::resonance, 0.45f }, { pid::filterDrv, 2.0f },
            { pid::filterEnvAmt, 0.85f }, { pid::keyTrack, 0.35f },
            { pid::ampA, 0.002f }, { pid::ampD, 0.28f }, { pid::ampS, 0.35f }, { pid::ampR, 0.1f },
            { pid::fegA, 0.001f }, { pid::fegD, 0.18f }, { pid::fegS, 0.0f }, { pid::fegR, 0.1f },
            { pid::voiceMode, 1 }, { pid::glide, 0.0f }, { pid::masterVol, 0.95f },
        }});

        // 6 - Hoover Stab: wide detuned saws with PW motion.
        list.push_back ({ "Hoover Stab", V {
            { pid::osc1Wave, 0 }, { pid::osc1Level, 0.7f },
            { pid::osc2Wave, 1 }, { pid::osc2Level, 0.6f }, { pid::osc2Semi, 7 },
            { pid::subLevel, 0.3f },
            { pid::uniVoices, 7 }, { pid::uniDetune, 0.6f }, { pid::uniWidth, 0.9f },
            { pid::drive, 0.35f },
            { pid::filterMode, 0 }, { pid::cutoff, 1600.0f }, { pid::resonance, 0.4f }, { pid::filterDrv, 1.8f },
            { pid::filterEnvAmt, 0.5f },
            { pid::lfo1Shape, 0 }, { pid::lfo1Rate, 6.0f },
            { pid::modSrc (0), (float) (int) ModSource::lfo1 }, { pid::modDst (0), (float) (int) ModDest::pulseWidth }, { pid::modAmt (0), 0.5f },
            { pid::masterVol, 0.85f },
        }});

        // 7 - Hard Sync Lead: osc2 synced + hard clip.
        list.push_back ({ "Hard Sync", V {
            { pid::osc1Wave, 0 }, { pid::osc1Level, 0.8f },
            { pid::osc2Wave, 0 }, { pid::osc2Level, 0.7f }, { pid::osc2Semi, 5 }, { pid::osc2Sync, 1 },
            { pid::subLevel, 0.35f },
            { pid::drive, 0.4f }, { pid::driveType, 1 },
            { pid::filterMode, 0 }, { pid::cutoff, 1400.0f }, { pid::resonance, 0.3f }, { pid::filterDrv, 2.0f },
            { pid::filterEnvAmt, 0.6f }, { pid::keyTrack, 0.5f },
            { pid::lfo2Shape, 0 }, { pid::lfo2Rate, 5.0f },
            { pid::modSrc (0), (float) (int) ModSource::lfo2 }, { pid::modDst (0), (float) (int) ModDest::osc2Pitch }, { pid::modAmt (0), 0.15f },
            { pid::voiceMode, 1 }, { pid::glide, 0.03f }, { pid::masterVol, 0.85f },
        }});

        // 8 - WT Morph: wavetable position swept by an envelope.
        list.push_back ({ "WT Morph", V {
            { pid::osc1Wave, 4 }, { pid::osc1Wt, 0.1f }, { pid::osc1Level, 0.85f },
            { pid::osc2Level, 0.0f }, { pid::subLevel, 0.5f },
            { pid::filterMode, 0 }, { pid::cutoff, 1200.0f }, { pid::resonance, 0.25f }, { pid::filterDrv, 1.5f },
            { pid::filterEnvAmt, 0.4f },
            { pid::ampA, 0.01f }, { pid::ampD, 0.7f }, { pid::ampS, 0.7f }, { pid::ampR, 0.3f },
            { pid::modSrc (0), (float) (int) ModSource::filterEnv }, { pid::modDst (0), (float) (int) ModDest::wavetablePos }, { pid::modAmt (0), 0.8f },
            { pid::modSrc (1), (float) (int) ModSource::lfo1 }, { pid::modDst (1), (float) (int) ModDest::wavetablePos }, { pid::modAmt (1), 0.2f },
            { pid::masterVol, 0.9f },
        }});

        // 9 - Bitcrush Dirt: lo-fi crushed bass.
        list.push_back ({ "Bitcrush Dirt", V {
            { pid::osc1Wave, 1 }, { pid::osc1Level, 0.85f },
            { pid::subWave, 1 }, { pid::subLevel, 0.5f },
            { pid::drive, 0.7f }, { pid::driveType, 3 },
            { pid::filterMode, 0 }, { pid::cutoff, 2500.0f }, { pid::resonance, 0.2f }, { pid::filterDrv, 1.4f },
            { pid::filterEnvAmt, 0.3f },
            { pid::voiceMode, 1 }, { pid::glide, 0.02f }, { pid::masterVol, 0.85f },
        }});

        // Artist-style banks (100 each), generated with musical parameter spreads.
        appendArtistBank (list, /*subtronics=*/false);  // Bassnectar-inspired
        appendArtistBank (list, /*subtronics=*/true);   // Subtronics-inspired

        return list;
    }

    // -----------------------------------------------------------------------
    // Procedurally generates a 100-patch bank in the flavour of a given artist.
    //
    // Bassnectar (subtronics=false): deep sub-forward, warm, halftime wobble,
    // detuned reese, softer saturation, more low-mid weight.
    // Subtronics  (subtronics=true): aggressive metallic growls, hard sync,
    // heavy hard/fold/bitcrush drive, fast wobbles, formant/wavetable morph.
    //
    // Values are chosen from musical ranges via a per-patch seeded LCG so the
    // bank is varied but every patch stays coherent and in range.
    // -----------------------------------------------------------------------
    static void appendArtistBank (std::vector<Preset>& list, bool subtronics)
    {
        static const char* bnDesc[] = {
            "Sub", "Cozza", "Halftime", "Cinema", "Warmth", "Reese", "Deep",
            "Lotus", "Void", "Nebula", "Bloom", "Freestyle", "Empathy",
            "Timestretch", "Wildstyle", "Glacial", "Migration", "Underground",
            "Speakerbox", "Ecstasy" };
        static const char* stDesc[] = {
            "Cyclops", "Tearout", "Riddim", "Scream", "Gnar", "Antifractal",
            "Fractal", "Laser", "Neuro", "Metal", "Growl", "Warp", "Nuclear",
            "Meltdown", "Titan", "Vortex", "Obliterate", "Cursed", "Mutant",
            "Havoc" };

        const char* const* desc = subtronics ? stDesc : bnDesc;
        const int   descN  = subtronics ? (int) (sizeof (stDesc) / sizeof (stDesc[0]))
                                        : (int) (sizeof (bnDesc) / sizeof (bnDesc[0]));
        const char* prefix = subtronics ? "Sub" : "Nectar";

        for (int i = 0; i < 100; ++i)
        {
            std::uint32_t s = (subtronics ? 0x53540001u : 0x424E0001u)
                            + (std::uint32_t) i * 2654435761u;

            auto nx = [&s]() { s = s * 1664525u + 1013904223u; return s; };
            auto rf = [&nx](float lo, float hi)
                      { return lo + (hi - lo) * (float) ((nx() >> 9) & 0x7fff) / 32767.0f; };
            auto pick = [&nx](std::initializer_list<int> opts)
                        { return *(opts.begin() + (nx() >> 9) % (std::uint32_t) opts.size()); };
            auto chance = [&rf](float p) { return rf (0.0f, 1.0f) < p; };

            Preset preset;
            preset.name = juce::String (prefix) + " " + desc[i % descN] + " "
                        + juce::String (i + 1).paddedLeft ('0', 3);

            V v;
            auto add = [&v] (const juce::String& id, float val) { v.push_back ({ id, val }); };

            if (! subtronics)
            {
                // ---- Bassnectar-inspired ----
                const int o1 = pick ({ 0, 0, 2, 4 });
                add (pid::osc1Wave, (float) o1);
                add (pid::osc1Level, rf (0.7f, 0.9f));
                add (pid::osc1Oct, (float) pick ({ -1, 0, 0 }));
                if (o1 == 4) add (pid::osc1Wt, rf (0.0f, 0.6f));

                const bool useO2 = chance (0.7f);
                add (pid::osc2Wave, (float) pick ({ 0, 1 }));
                add (pid::osc2Level, useO2 ? rf (0.4f, 0.75f) : 0.0f);
                add (pid::osc2Oct, (float) pick ({ -1, 0 }));
                add (pid::osc2Fine, rf (-14.0f, 14.0f));

                add (pid::subWave, 0.0f);
                add (pid::subOct, (float) pick ({ -1, -1, -2 }));
                add (pid::subLevel, rf (0.55f, 0.85f));
                add (pid::noiseLevel, chance (0.3f) ? rf (0.02f, 0.1f) : 0.0f);

                add (pid::uniVoices, (float) pick ({ 1, 2, 3, 3, 5 }));
                add (pid::uniDetune, rf (0.1f, 0.5f));
                add (pid::uniWidth, rf (0.4f, 0.9f));

                add (pid::drive, rf (0.1f, 0.45f));
                add (pid::driveType, (float) pick ({ 0, 0, 2 }));

                add (pid::filterMode, 0.0f);
                add (pid::cutoff, rf (300.0f, 1500.0f));
                add (pid::resonance, rf (0.15f, 0.5f));
                add (pid::filterDrv, rf (1.2f, 2.6f));
                add (pid::keyTrack, rf (0.2f, 0.5f));
                add (pid::filterEnvAmt, rf (0.2f, 0.7f));

                add (pid::ampA, rf (0.002f, 0.02f));
                add (pid::ampD, rf (0.2f, 0.7f));
                add (pid::ampS, rf (0.5f, 0.9f));
                add (pid::ampR, rf (0.1f, 0.4f));

                add (pid::fegA, rf (0.001f, 0.01f));
                add (pid::fegD, rf (0.15f, 0.4f));
                add (pid::fegS, rf (0.05f, 0.3f));
                add (pid::fegR, rf (0.1f, 0.3f));

                const bool sync1 = chance (0.5f);
                add (pid::lfo1Shape, (float) pick ({ 0, 1 }));
                add (pid::lfo1Sync, sync1 ? 1.0f : 0.0f);
                add (pid::lfo1Rate, sync1 ? rf (0.5f, 4.0f) : rf (0.5f, 6.0f));

                add (pid::voiceMode, (float) pick ({ 1, 1, 2 }));
                add (pid::glide, rf (0.0f, 0.1f));
                add (pid::masterVol, rf (0.85f, 0.95f));

                add (pid::modSrc (0), (float) (int) ModSource::lfo1);
                add (pid::modDst (0), (float) (int) ModDest::cutoff);
                add (pid::modAmt (0), rf (0.2f, 0.6f));
                if (chance (0.6f))
                {
                    add (pid::modSrc (1), (float) (int) ModSource::modWheel);
                    add (pid::modDst (1), (float) (int) ModDest::cutoff);
                    add (pid::modAmt (1), rf (0.2f, 0.6f));
                }
                if (chance (0.3f))
                {
                    add (pid::modSrc (2), (float) (int) ModSource::lfo2);
                    add (pid::modDst (2), (float) (int) ModDest::osc1Pitch);
                    add (pid::modAmt (2), rf (0.01f, 0.05f));
                }
            }
            else
            {
                // ---- Subtronics-inspired ----
                const int o1 = pick ({ 0, 4, 1 });
                add (pid::osc1Wave, (float) o1);
                add (pid::osc1Level, rf (0.8f, 0.95f));
                if (o1 == 4) add (pid::osc1Wt, rf (0.0f, 1.0f));

                add (pid::osc2Wave, (float) pick ({ 0, 1 }));
                add (pid::osc2Level, rf (0.4f, 0.8f));
                add (pid::osc2Semi, (float) pick ({ 0, 5, 7, 12 }));
                add (pid::osc2Fine, rf (-20.0f, 20.0f));
                add (pid::osc2Sync, chance (0.6f) ? 1.0f : 0.0f);

                add (pid::subWave, (float) pick ({ 0, 1 }));
                add (pid::subOct, (float) pick ({ -1, -1, -2 }));
                add (pid::subLevel, rf (0.3f, 0.6f));
                add (pid::noiseLevel, chance (0.4f) ? rf (0.03f, 0.15f) : 0.0f);

                add (pid::uniVoices, (float) pick ({ 1, 3, 5, 7 }));
                add (pid::uniDetune, rf (0.2f, 0.7f));
                add (pid::uniWidth, rf (0.5f, 1.0f));

                add (pid::drive, rf (0.4f, 0.85f));
                add (pid::driveType, (float) pick ({ 1, 2, 3 }));

                add (pid::filterMode, (float) pick ({ 0, 0, 3 }));
                add (pid::cutoff, rf (500.0f, 3500.0f));
                add (pid::resonance, rf (0.4f, 0.85f));
                add (pid::filterDrv, rf (2.0f, 5.0f));
                add (pid::keyTrack, rf (0.3f, 0.6f));
                add (pid::filterEnvAmt, rf (0.4f, 0.9f));

                add (pid::ampA, rf (0.001f, 0.008f));
                add (pid::ampD, rf (0.15f, 0.5f));
                add (pid::ampS, rf (0.3f, 0.8f));
                add (pid::ampR, rf (0.05f, 0.25f));

                add (pid::fegA, rf (0.0005f, 0.006f));
                add (pid::fegD, rf (0.1f, 0.35f));
                add (pid::fegS, rf (0.0f, 0.25f));
                add (pid::fegR, rf (0.08f, 0.25f));

                const bool sync1 = chance (0.6f);
                add (pid::lfo1Shape, (float) pick ({ 0, 1, 3, 4 }));
                add (pid::lfo1Sync, sync1 ? 1.0f : 0.0f);
                add (pid::lfo1Rate, sync1 ? rf (1.0f, 8.0f) : rf (3.0f, 16.0f));
                add (pid::lfo2Shape, (float) pick ({ 0, 1, 4 }));
                add (pid::lfo2Rate, rf (0.5f, 6.0f));

                add (pid::voiceMode, 1.0f);
                add (pid::glide, rf (0.0f, 0.05f));
                add (pid::masterVol, rf (0.8f, 0.9f));

                add (pid::modSrc (0), (float) (int) ModSource::lfo1);
                add (pid::modDst (0), (float) (int) ModDest::cutoff);
                add (pid::modAmt (0), rf (0.5f, 0.9f));

                add (pid::modSrc (1), (float) (int) ModSource::lfo2);
                add (pid::modDst (1), (float) (o1 == 4 ? (int) ModDest::wavetablePos
                                                       : (int) ModDest::pulseWidth));
                add (pid::modAmt (1), rf (0.3f, 0.8f));

                if (chance (0.4f))
                {
                    add (pid::modSrc (2), (float) (int) ModSource::filterEnv);
                    add (pid::modDst (2), (float) (int) ModDest::cutoff);
                    add (pid::modAmt (2), rf (0.3f, 0.7f));
                }
                if (chance (0.3f))
                {
                    add (pid::modSrc (3), (float) (int) ModSource::lfo2);
                    add (pid::modDst (3), (float) (int) ModDest::resonance);
                    add (pid::modAmt (3), rf (0.1f, 0.4f));
                }
            }

            preset.values = std::move (v);
            list.push_back (std::move (preset));
        }
    }

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Preset> presets;
    int currentIndex = 0;
};
} // namespace bassforge
