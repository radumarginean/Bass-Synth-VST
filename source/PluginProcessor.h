#pragma once

#include <JuceHeader.h>
#include "dsp/SynthEngine.h"
#include "dsp/Patch.h"
#include "dsp/Parameters.h"

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

    // Shared metering value for the editor (peak of last block, atomic).
    std::atomic<float> outputLevel { 0.0f };

private:
    void buildPatch (bassforge::Patch& patch, double bpm);
    float resolveLfoRate (const char* rateId, const char* syncId, double bpm) const;

    juce::AudioProcessorValueTreeState apvts;
    bassforge::SynthEngine engine;

    // Output stage
    juce::dsp::IIR::Filter<float> dcBlockerL, dcBlockerR;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassForgeAudioProcessor)
};
