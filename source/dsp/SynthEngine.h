#pragma once

#include "BassVoice.h"
#include "Patch.h"
#include <vector>

namespace bassforge
{
/**
    Voice allocator and MIDI handler.

    Supports polyphonic, monophonic (always retrigger) and legato (glide, no
    retrigger while notes overlap) modes with a held-note stack, plus glide,
    pitch bend and mod wheel. MIDI is handled sample-accurately by splitting
    each block at event boundaries.
*/
class SynthEngine
{
public:
    static constexpr int numVoices = 16;

    void prepare (double sampleRate, int /*blockSize*/)
    {
        voices.resize (numVoices);
        for (auto& v : voices)
            v.prepare (sampleRate, &wavetable);
        heldNotes.clear();
        orderCounter = 0;
        modWheel = 0.0f;
        pitchBendNorm = 0.0f;
    }

    void reset()
    {
        for (auto& v : voices)
            v.kill();
        heldNotes.clear();
    }

    void render (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Patch& patch)
    {
        auto* left  = buffer.getWritePointer (0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

        int samplePos = 0;
        for (const auto meta : midi)
        {
            const int eventPos = meta.samplePosition;
            if (eventPos > samplePos)
            {
                renderChunk (left, right, samplePos, eventPos - samplePos, patch);
                samplePos = eventPos;
            }
            handleMidi (meta.getMessage(), patch);
        }

        if (samplePos < buffer.getNumSamples())
            renderChunk (left, right, samplePos, buffer.getNumSamples() - samplePos, patch);
    }

private:
    void renderChunk (float* left, float* right, int start, int count, const Patch& patch)
    {
        const float bendSemis = pitchBendNorm * (float) patch.bendRange;
        for (auto& v : voices)
            if (v.isActive())
                v.render (left + start, right + start, count, patch, modWheel, bendSemis);
    }

    void handleMidi (const juce::MidiMessage& m, const Patch& patch)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity(), patch);
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber(), patch);
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            heldNotes.clear();
            for (auto& v : voices) v.stopNote();
        }
        else if (m.isPitchWheel())
            pitchBendNorm = (m.getPitchWheelValue() - 8192) / 8192.0f;
        else if (m.isController() && m.getControllerNumber() == 1)
            modWheel = m.getControllerValue() / 127.0f;
    }

    void noteOn (int note, float vel, const Patch& patch)
    {
        if (patch.voiceMode == VoiceMode::poly)
        {
            auto* v = allocateVoice();
            v->setStartOrder (++orderCounter);
            v->startNote (note, vel, false, (double) note);
            return;
        }

        // Mono / legato: a single voice, note stack
        const bool wasHeld = ! heldNotes.empty();
        heldNotes.push_back (note);

        auto* v = &voices[0];
        const bool legatoGlide = (patch.voiceMode == VoiceMode::legato) && wasHeld && v->isActive();
        v->setStartOrder (++orderCounter);
        v->startNote (note, vel, legatoGlide, v->isActive() ? (double) v->getNote() : (double) note);
    }

    void noteOff (int note, const Patch& patch)
    {
        if (patch.voiceMode == VoiceMode::poly)
        {
            for (auto& v : voices)
                if (v.isActive() && ! v.isReleasing() && v.getNote() == note)
                    v.stopNote();
            return;
        }

        // Remove from held stack
        for (int i = (int) heldNotes.size() - 1; i >= 0; --i)
            if (heldNotes[(size_t) i] == note) { heldNotes.erase (heldNotes.begin() + i); break; }

        auto* v = &voices[0];
        if (heldNotes.empty())
        {
            v->stopNote();
        }
        else
        {
            const int back = heldNotes.back();
            const bool glide = (patch.voiceMode == VoiceMode::legato);
            v->startNote (back, 1.0f, glide, (double) v->getNote());
        }
    }

    BassVoice* allocateVoice()
    {
        // Prefer a free voice
        for (auto& v : voices)
            if (! v.isActive())
                return &v;

        // Then the oldest releasing voice
        BassVoice* best = nullptr;
        for (auto& v : voices)
            if (v.isReleasing())
                if (best == nullptr || v.getStartOrder() < best->getStartOrder())
                    best = &v;
        if (best != nullptr)
            return best;

        // Otherwise steal the oldest voice overall
        best = &voices[0];
        for (auto& v : voices)
            if (v.getStartOrder() < best->getStartOrder())
                best = &v;
        return best;
    }

    Wavetable              wavetable;
    std::vector<BassVoice>  voices;
    std::vector<int>        heldNotes;
    juce::uint32            orderCounter = 0;
    float                   modWheel      = 0.0f;
    float                   pitchBendNorm = 0.0f;
};
} // namespace bassforge
