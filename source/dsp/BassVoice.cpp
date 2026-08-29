#include "BassVoice.h"

namespace bassforge
{
static inline float clampf (float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

void BassVoice::prepare (double newSampleRate, const Wavetable* sharedTable)
{
    sampleRate = newSampleRate;
    wavetable  = sharedTable;

    for (auto& o : osc1) o.prepare (sampleRate, sharedTable);
    for (auto& o : osc2) o.prepare (sampleRate, sharedTable);
    subOsc.prepare (sampleRate, sharedTable);

    filterL.prepare (sampleRate);
    filterR.prepare (sampleRate);
    ampEnv.prepare (sampleRate);
    filtEnv.prepare (sampleRate);
    lfo1.prepare (sampleRate);
    lfo2.prepare (sampleRate);
}

void BassVoice::startNote (int midiNote, float vel, bool doGlide, double glideFromNote)
{
    currentMidiNote = midiNote;
    targetNote      = (double) midiNote;
    velocity        = clampf (vel, 0.0f, 1.0f);

    if (doGlide)
        currentNote = glideFromNote;   // slide from the previous pitch
    else
    {
        currentNote = targetNote;

        // Fresh note: reset oscillator/filter/LFO state and randomise phases
        for (auto& o : osc1) { o.reset(); o.setPhase (noiseRng.nextFloat()); }
        for (auto& o : osc2) { o.reset(); o.setPhase (noiseRng.nextFloat()); }
        subOsc.reset();
        filterL.reset();
        filterR.reset();
        lfo1.reset (0.0);
        lfo2.reset (0.0);
    }

    ampEnv.noteOn();
    filtEnv.noteOn();
    active    = true;
    releasing = false;
}

void BassVoice::stopNote()
{
    ampEnv.noteOff();
    filtEnv.noteOff();
    releasing = true;
}

void BassVoice::kill()
{
    active    = false;
    releasing = false;
    ampEnv.reset();
    filtEnv.reset();
    currentMidiNote = -1;
}

void BassVoice::updateUnison (const Patch& patch) noexcept
{
    uniCount = juce::jlimit (1, maxUnison, patch.uniVoices);

    const float maxCents = 50.0f;
    for (int i = 0; i < uniCount; ++i)
    {
        const float spread = (uniCount == 1) ? 0.0f
                           : ((float) i / (float) (uniCount - 1)) * 2.0f - 1.0f;

        const float cents = spread * patch.uniDetune * maxCents;
        uniRatio[i] = std::pow (2.0f, cents / 1200.0f);

        // Equal-power pan from the stereo width control.
        const float pan   = spread * patch.uniWidth;
        const float angle = (pan * 0.5f + 0.5f) * (juce::MathConstants<float>::pi * 0.5f);
        uniGainL[i] = std::cos (angle);
        uniGainR[i] = std::sin (angle);
    }
}

float BassVoice::applyDrive (float x, float amount, DriveType type) const noexcept
{
    if (amount <= 0.0001f)
        return x;

    const float g = 1.0f + amount * 8.0f;

    switch (type)
    {
        case DriveType::soft:
            // tanh soft clip, normalised so the peak stays near unity
            return std::tanh (x * g) / std::tanh (g);

        case DriveType::hard:
            // aggressive clip with makeup so it doesn't collapse in level
            return clampf (x * g, -1.0f, 1.0f);

        case DriveType::fold:
            // sinusoidal wavefolder
            return std::sin (x * g * 1.5f);

        case DriveType::bitcrush:
        {
            const float bits  = 16.0f - amount * 13.0f;
            const float steps = std::pow (2.0f, bits);
            return std::round (x * steps) / steps;
        }

        default:
            return x;
    }
}

void BassVoice::render (float* left, float* right, int numSamples,
                        const Patch& patch, float modWheel, float pitchBendSemis)
{
    if (! active)
        return;

    // --- per-block configuration -----------------------------------------
    updateUnison (patch);

    for (int i = 0; i < uniCount; ++i)
    {
        osc1[i].setWave (patch.osc1Wave);
        osc1[i].setPulseWidth (patch.osc1Pw);
        osc1[i].setWavetablePos (patch.osc1Wt);
        osc2[i].setWave (patch.osc2Wave);
        osc2[i].setPulseWidth (patch.osc2Pw);
        osc2[i].setWavetablePos (patch.osc2Wt);
    }
    subOsc.setWave (patch.subWave == 0 ? OscWave::sine : OscWave::square);

    filterL.setMode (patch.filterMode);
    filterR.setMode (patch.filterMode);

    ampEnv.setParameters (patch.ampA, patch.ampD, patch.ampS, patch.ampR);
    filtEnv.setParameters (patch.fegA, patch.fegD, patch.fegS, patch.fegR);

    lfo1.setShape (patch.lfo1Shape);
    lfo2.setShape (patch.lfo2Shape);
    lfo1.setRate (patch.lfo1Sync ? patch.lfo1Rate : patch.lfo1Rate); // rate already resolved by processor
    lfo2.setRate (patch.lfo2Sync ? patch.lfo2Rate : patch.lfo2Rate);

    // Glide coefficient
    const float glideSeconds = patch.glide * 1.5f;
    glideCoeff = glideSeconds > 0.0005f
                     ? 1.0 - std::exp (-1.0 / (sampleRate * glideSeconds))
                     : 1.0;

    const float uniComp = 1.0f / std::sqrt ((float) uniCount);

    // --- sample loop ------------------------------------------------------
    for (int n = 0; n < numSamples; ++n)
    {
        // Glide toward target
        currentNote += glideCoeff * (targetNote - currentNote);

        // Modulation sources
        const float ampVal = ampEnv.process();
        const float fegVal = filtEnv.process();
        const float l1     = lfo1.process();
        const float l2     = lfo2.process();

        if (! ampEnv.isActive())
        {
            active    = false;
            releasing = false;
            currentMidiNote = -1;
            break;
        }

        float srcVal[(int) ModSource::numSources] = { 0.0f };
        srcVal[(int) ModSource::none]      = 0.0f;
        srcVal[(int) ModSource::lfo1]      = l1;
        srcVal[(int) ModSource::lfo2]      = l2;
        srcVal[(int) ModSource::filterEnv] = fegVal;
        srcVal[(int) ModSource::ampEnv]    = ampVal;
        srcVal[(int) ModSource::velocity]  = velocity;
        srcVal[(int) ModSource::modWheel]  = modWheel;
        srcVal[(int) ModSource::keyTrack]  = (float) ((currentNote - 60.0) / 60.0);

        float dst[(int) ModDest::numDests] = { 0.0f };
        for (const auto& r : patch.routes)
        {
            if (r.src == ModSource::none || r.dst == ModDest::none || r.amt == 0.0f)
                continue;
            dst[(int) r.dst] += srcVal[(int) r.src] * r.amt;
        }

        // Resolve pitch (semitones)
        const double glob = pitchBendSemis;
        const double n1   = currentNote + glob + patch.osc1Oct * 12.0 + patch.osc1Semi
                          + patch.osc1Fine * 0.01 + dst[(int) ModDest::osc1Pitch] * 12.0;
        const double n2   = currentNote + glob + patch.osc2Oct * 12.0 + patch.osc2Semi
                          + patch.osc2Fine * 0.01 + dst[(int) ModDest::osc2Pitch] * 12.0;
        const double nSub = currentNote + glob + patch.subOct * 12.0;

        const double f1   = noteToFreq (n1);
        const double f2   = noteToFreq (n2);
        const double fSub = noteToFreq (nSub);

        // Oscillator levels (+ modulation)
        const float lvl1 = clampf (patch.osc1Level + dst[(int) ModDest::osc1Level], 0.0f, 1.5f);
        const float lvl2 = clampf (patch.osc2Level + dst[(int) ModDest::osc2Level], 0.0f, 1.5f);

        // Pulse width / WT position modulation applied to the shared oscillators
        if (dst[(int) ModDest::pulseWidth] != 0.0f)
        {
            const float pw1 = clampf (patch.osc1Pw + dst[(int) ModDest::pulseWidth] * 0.48f, 0.02f, 0.98f);
            const float pw2 = clampf (patch.osc2Pw + dst[(int) ModDest::pulseWidth] * 0.48f, 0.02f, 0.98f);
            for (int i = 0; i < uniCount; ++i) { osc1[i].setPulseWidth (pw1); osc2[i].setPulseWidth (pw2); }
        }
        if (dst[(int) ModDest::wavetablePos] != 0.0f)
        {
            const float wt1 = clampf (patch.osc1Wt + dst[(int) ModDest::wavetablePos], 0.0f, 1.0f);
            const float wt2 = clampf (patch.osc2Wt + dst[(int) ModDest::wavetablePos], 0.0f, 1.0f);
            for (int i = 0; i < uniCount; ++i) { osc1[i].setWavetablePos (wt1); osc2[i].setWavetablePos (wt2); }
        }

        float preL = 0.0f, preR = 0.0f;

        for (int i = 0; i < uniCount; ++i)
        {
            const float s1 = osc1[i].process (f1 * uniRatio[i]) * lvl1;

            // Hard sync osc2 to osc1's fundamental (unison voice by voice)
            if (patch.osc2Sync && osc1[i].wrapped())
                osc2[i].hardSync();

            const float s2 = osc2[i].process (f2 * uniRatio[i]) * lvl2;

            const float s = s1 + s2;
            preL += s * uniGainL[i];
            preR += s * uniGainR[i];
        }

        preL *= uniComp;
        preR *= uniComp;

        // Sub + noise (centred)
        const float sub   = subOsc.process (fSub) * patch.subLevel;
        const float noise = (noiseRng.nextFloat() * 2.0f - 1.0f) * patch.noiseLevel;
        const float mono  = sub + noise;
        preL += mono;
        preR += mono;

        // Pre-filter drive
        preL = applyDrive (preL, patch.drive, patch.driveType);
        preR = applyDrive (preR, patch.drive, patch.driveType);

        // Cutoff with key-track, filter envelope and modulation (in octaves)
        double octaves = patch.filterEnvAmt * fegVal * 5.0
                       + patch.keyTrack * (currentNote - 60.0) / 12.0
                       + dst[(int) ModDest::cutoff] * 6.0;
        float cutoffHz = clampf (patch.cutoff * (float) std::pow (2.0, octaves), 20.0f, 18000.0f);
        const float res = clampf (patch.resonance + dst[(int) ModDest::resonance], 0.0f, 1.0f);

        filterL.setParams (cutoffHz, res, patch.filterDrive);
        filterR.setParams (cutoffHz, res, patch.filterDrive);

        float outL = filterL.process (preL);
        float outR = filterR.process (preR);

        // Amp: envelope * velocity * amplitude modulation
        const float ampMod = clampf (1.0f + dst[(int) ModDest::amplitude], 0.0f, 2.0f);
        const float gain   = ampVal * (0.4f + 0.6f * velocity) * ampMod;
        outL *= gain;
        outR *= gain;

        // Global pan modulation
        const float pan = dst[(int) ModDest::pan];
        if (pan != 0.0f)
        {
            const float a  = clampf (pan * 0.5f + 0.5f, 0.0f, 1.0f) * (juce::MathConstants<float>::pi * 0.5f);
            outL *= std::cos (a) * 1.41421356f;
            outR *= std::sin (a) * 1.41421356f;
        }

        left[n]  += outL;
        right[n] += outR;
    }
}
} // namespace bassforge
