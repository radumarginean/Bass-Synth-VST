// Offline smoke test: drive the synth engine with MIDI, render audio and
// assert the output is finite and audible. Not a full unit-test suite - a fast
// guard that the DSP graph runs end to end without exploding.

#include <JuceHeader.h>
#include "dsp/SynthEngine.h"
#include "dsp/Patch.h"
#include "dsp/Parameters.h"
#include "Presets.h"
#include <cmath>
#include <cstdio>

using namespace bassforge;

// ---------------------------------------------------------------------------
// A minimal AudioProcessor host just so we can spin up an APVTS + PresetManager
// and validate the factory presets without the full plugin/editor.
// ---------------------------------------------------------------------------
class DummyProcessor : public juce::AudioProcessor
{
public:
    DummyProcessor() : apvts (*this, nullptr, "test", createParameterLayout()) {}

    const juce::String getName() const override { return "dummy"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;
};

static int testPresets()
{
    DummyProcessor proc;
    PresetManager mgr (proc.apvts);

    int failures = 0;

    // Every parameter id referenced by a preset must exist.
    for (const auto& preset : mgr.getPresets())
        for (const auto& kv : preset.values)
            if (proc.apvts.getParameter (kv.first) == nullptr)
            {
                std::printf ("  FAIL: preset '%s' references unknown param '%s'\n",
                             preset.name.toRawUTF8(), kv.first.toRawUTF8());
                ++failures;
            }

    // Loading each preset must leave all parameters finite and in range.
    for (int i = 0; i < mgr.getNumPresets(); ++i)
    {
        mgr.loadPreset (i);
        for (auto* p : proc.getParameters())
        {
            const float norm = p->getValue();
            if (! std::isfinite (norm) || norm < 0.0f || norm > 1.0f)
            {
                std::printf ("  FAIL: preset %d produced out-of-range value\n", i);
                ++failures;
                break;
            }
        }
    }

    std::printf ("  [presets       ] %d presets checked\n", mgr.getNumPresets());
    return failures;
}

static int runCase (const char* name, Patch patch)
{
    const double sr = 48000.0;
    const int    block = 512;

    SynthEngine engine;
    engine.prepare (sr, block);

    juce::AudioBuffer<float> buffer (2, block);

    double peak = 0.0, rms = 0.0;
    long   count = 0;
    bool   nan = false;

    const int totalBlocks = (int) (2.0 * sr / block);
    for (int b = 0; b < totalBlocks; ++b)
    {
        buffer.clear();
        juce::MidiBuffer midi;

        if (b == 2)
        {
            midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 110), 0);   // C1
            midi.addEvent (juce::MidiMessage::noteOn (1, 43, (juce::uint8) 90), 16);   // G1
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 24);        // mod wheel
        }
        if (b == totalBlocks - 30)
        {
            midi.addEvent (juce::MidiMessage::noteOff (1, 36), 0);
            midi.addEvent (juce::MidiMessage::noteOff (1, 43), 0);
        }

        engine.render (buffer, midi, patch);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* d = buffer.getReadPointer (ch);
            for (int i = 0; i < block; ++i)
            {
                const float v = d[i];
                if (! std::isfinite (v)) nan = true;
                peak = juce::jmax (peak, (double) std::abs (v));
                rms += (double) v * v;
                ++count;
            }
        }
    }

    rms = std::sqrt (rms / (double) juce::jmax (1L, count));
    std::printf ("  [%-14s] peak=%.4f rms=%.5f nan=%s\n", name, peak, rms, nan ? "YES" : "no");

    if (nan)       { std::printf ("  FAIL(%s): non-finite output\n", name); return 1; }
    if (peak < 0.005) { std::printf ("  FAIL(%s): essentially silent\n", name); return 1; }
    if (peak > 20.0)  { std::printf ("  FAIL(%s): output diverged\n", name); return 1; }
    return 0;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit; // APVTS needs a message manager
    int failures = 0;

    failures += testPresets();

    // 1) Default-ish patch
    {
        Patch p;
        p.osc1Level = 0.8f; p.subLevel = 0.5f;
        failures += runCase ("default", p);
    }

    // 2) Wavetable + heavy unison + high resonance
    {
        Patch p;
        p.osc1Wave = OscWave::wavetable; p.osc1Wt = 0.6f; p.osc1Level = 0.7f;
        p.osc2Wave = OscWave::saw; p.osc2Level = 0.5f;
        p.uniVoices = 7; p.uniDetune = 0.6f; p.uniWidth = 0.8f;
        p.resonance = 0.9f; p.cutoff = 800.0f; p.filterDrive = 3.0f; p.drive = 0.5f;
        failures += runCase ("wt+unison+res", p);
    }

    // 3) Mono/legato with glide and mod matrix routing
    {
        Patch p;
        p.voiceMode = VoiceMode::legato; p.glide = 0.4f;
        p.osc1Level = 0.8f;
        p.routes[0] = { ModSource::lfo1, ModDest::cutoff, 0.5f };
        p.routes[1] = { ModSource::filterEnv, ModDest::cutoff, 0.8f };
        p.routes[2] = { ModSource::lfo2, ModDest::osc1Pitch, 0.02f };
        p.driveType = DriveType::fold; p.drive = 0.7f;
        failures += runCase ("legato+mod", p);
    }

    // 4) All drive types shouldn't blow up
    for (int dt = 0; dt < 4; ++dt)
    {
        Patch p; p.drive = 0.9f; p.driveType = (DriveType) dt; p.osc1Level = 0.9f;
        failures += runCase (juce::String ("drive#" + juce::String (dt)).toRawUTF8(), p);
    }

    if (failures == 0) { std::puts ("PASS"); return 0; }
    std::printf ("FAILED (%d cases)\n", failures);
    return 1;
}
