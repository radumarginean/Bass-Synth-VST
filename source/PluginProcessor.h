#pragma once

#include <JuceHeader.h>
#include <array>
#include "dsp/SynthEngine.h"
#include "dsp/Patch.h"
#include "dsp/Parameters.h"
#include "Presets.h"

/**
    BassForge - a bass-focused subtractive/wavetable synthesizer.

    The DSP architecture draws on two open-source synths: the band-limited
    wavetable + mod-matrix approach of Surge XT, and the dual-oscillator,
    sub-heavy, drive-into-ladder voicing of Vaporizer2. It is tuned for the low
    end: sub oscillator, unison, non-linear ladder filter with drive, dual
    envelopes, dual LFOs and mono/legato glide.
*/
class BassForgeAudioProcessor : public juce::AudioProcessor
{
public:
    BassForgeAudioProcessor();
    ~BassForgeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BassForge"; }
    bool acceptsMidi()  const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }
    bassforge::PresetManager& getPresetManager() { return presets; }

    // Shared metering value for the editor (peak of last block, atomic).
    std::atomic<float> outputLevel { 0.0f };

    // ---- Visualizer tap ---------------------------------------------------
    // A lock-free-ish mono ring buffer the editor reads for the scope/FFT.
    static constexpr int scopeSize = 1 << 13; // 8192 samples
    void readScope (float* dest, int numSamples) const noexcept;

private:
    void buildPatch (bassforge::Patch& patch, double bpm);
    float resolveLfoRate (const char* rateId, const char* syncId, double bpm) const;

    juce::AudioProcessorValueTreeState apvts;
    bassforge::SynthEngine engine;
    bassforge::PresetManager presets { apvts };

    // Visualizer ring buffer
    std::array<float, scopeSize> scopeBuffer {};
    std::atomic<int> scopeWritePos { 0 };

    // Output stage
    juce::dsp::IIR::Filter<float> dcBlockerL, dcBlockerR;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassForgeAudioProcessor)
};
