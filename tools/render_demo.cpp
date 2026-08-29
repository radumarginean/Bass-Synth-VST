// Renders a short bass line to a WAV file so the engine can be auditioned
// without a host. Usage: render_demo <out.wav>

#include <JuceHeader.h>
#include "dsp/SynthEngine.h"
#include "dsp/Patch.h"

using namespace bassforge;

int main (int argc, char** argv)
{
    const juce::String outPath = argc > 1 ? juce::String (argv[1]) : "demo.wav";

    const double sr = 48000.0;
    const int    block = 256;

    SynthEngine engine;
    engine.prepare (sr, block);

    // A punchy, driven reese-ish bass patch.
    Patch p;
    p.osc1Wave = OscWave::saw;       p.osc1Level = 0.85f;
    p.osc2Wave = OscWave::saw;       p.osc2Level = 0.7f; p.osc2Fine = 12.0f; p.osc2Oct = 0;
    p.subWave = 0; p.subOct = -1;    p.subLevel = 0.6f;
    p.uniVoices = 5; p.uniDetune = 0.35f; p.uniWidth = 0.7f;
    p.drive = 0.35f; p.driveType = DriveType::soft;
    p.filterMode = FilterMode::lowpass24;
    p.cutoff = 350.0f; p.resonance = 0.35f; p.filterDrive = 2.2f;
    p.keyTrack = 0.4f; p.filterEnvAmt = 0.75f;
    p.ampA = 0.003f; p.ampD = 0.5f; p.ampS = 0.7f; p.ampR = 0.12f;
    p.fegA = 0.002f; p.fegD = 0.28f; p.fegS = 0.1f; p.fegR = 0.15f;
    p.voiceMode = VoiceMode::mono; p.glide = 0.06f;
    p.lfo1Shape = LfoShape::sine; p.lfo1Rate = 5.0f;
    p.routes[0] = { ModSource::lfo1, ModDest::cutoff, 0.15f };
    p.masterVol = 1.0f;

    // 16-step riff (MIDI notes), each an eighth note at 128 BPM.
    const int riff[] = { 36, 36, 48, 36, 43, 36, 46, 36,
                         36, 36, 48, 41, 43, 36, 39, 34 };
    const double bpm = 128.0;
    const double stepSec = 60.0 / bpm / 2.0;      // eighth note
    const int    stepSamples = (int) (stepSec * sr);
    const int    gate = (int) (stepSamples * 0.9);

    const int totalSamples = stepSamples * (int) std::size (riff) + (int) (0.4 * sr);

    juce::AudioBuffer<float> out (2, totalSamples);
    out.clear();

    juce::AudioBuffer<float> buffer (2, block);

    int written = 0;
    int nextStep = 0, stepIndex = 0, gateOffAt = -1, lastNote = -1;

    while (written < totalSamples)
    {
        const int n = juce::jmin (block, totalSamples - written);
        buffer.clear();
        juce::MidiBuffer midi;

        for (int i = 0; i < n; ++i)
        {
            const int globalPos = written + i;
            if (stepIndex < (int) std::size (riff) && globalPos >= nextStep)
            {
                const int note = riff[stepIndex];
                midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 112), i);
                lastNote  = note;
                gateOffAt = nextStep + gate;
                nextStep += stepSamples;
                ++stepIndex;
            }
            if (gateOffAt >= 0 && globalPos == gateOffAt && lastNote >= 0)
            {
                midi.addEvent (juce::MidiMessage::noteOff (1, lastNote), i);
                gateOffAt = -1;
            }
        }

        juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), 2, n);
        engine.render (sub, midi, p);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, written, buffer, ch, 0, n);

        written += n;
    }

    // Master: DC block + soft clip + gain (mirrors the plugin output stage).
    juce::dsp::IIR::Filter<float> dc[2];
    auto hp = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 18.0f);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) totalSamples, 1 };
    for (auto& f : dc) { f.coefficients = hp; f.prepare (spec); }

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = out.getWritePointer (ch);
        for (int i = 0; i < totalSamples; ++i)
            d[i] = std::tanh (dc[ch].processSample (d[i]) * 1.0f);
    }

    juce::File file (juce::File::getCurrentWorkingDirectory().getChildFile (outPath));
    file.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr) { juce::Logger::writeToLog ("cannot open output"); return 1; }

    std::unique_ptr<juce::AudioFormatWriter> writer (
        wav.createWriterFor (stream.release(), sr, 2, 24, {}, 0));
    if (writer == nullptr) { juce::Logger::writeToLog ("no writer"); return 1; }

    writer->writeFromAudioSampleBuffer (out, 0, totalSamples);
    writer.reset();

    juce::Logger::writeToLog ("wrote " + file.getFullPathName()
                              + "  (" + juce::String (totalSamples / sr, 2) + "s)");
    return 0;
}
