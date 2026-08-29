#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/PatchBuilder.h"

using namespace bassforge;

BassForgeAudioProcessor::BassForgeAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BassForge", createParameterLayout())
{
}

void BassForgeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;

    // ~20 Hz high-pass to remove DC / rumble from heavy drive.
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 18.0f);
    dcBlockerL.coefficients = coeffs;
    dcBlockerR.coefficients = coeffs;
    dcBlockerL.prepare (spec);
    dcBlockerR.prepare (spec);
}

bool BassForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BassForgeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    buffer.clear();

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;
    }

    Patch patch;
    bassforge::buildPatch (apvts, patch, bpm);

    engine.render (buffer, midi, patch);

    // Output stage: DC block + master volume + soft safety clip.
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    const float vol = patch.masterVol;

    float peak = 0.0f;
    int wp = scopeWritePos.load (std::memory_order_relaxed);
    constexpr int mask = scopeSize - 1;

    for (int n = 0; n < buffer.getNumSamples(); ++n)
    {
        float l = dcBlockerL.processSample (left[n]) * vol;
        l = std::tanh (l);
        left[n] = l;
        peak = juce::jmax (peak, std::abs (l));

        float mono = l;
        if (right != nullptr)
        {
            float r = dcBlockerR.processSample (right[n]) * vol;
            r = std::tanh (r);
            right[n] = r;
            peak = juce::jmax (peak, std::abs (r));
            mono = 0.5f * (l + r);
        }

        scopeBuffer[(size_t) (wp & mask)] = mono;
        ++wp;
    }

    scopeWritePos.store (wp & mask, std::memory_order_release);
    outputLevel.store (peak);
}

void BassForgeAudioProcessor::readScope (float* dest, int numSamples) const noexcept
{
    numSamples = juce::jmin (numSamples, scopeSize);
    constexpr int mask = scopeSize - 1;
    const int wp = scopeWritePos.load (std::memory_order_acquire);
    // Copy the most recent numSamples in chronological order.
    const int start = wp - numSamples;
    for (int i = 0; i < numSamples; ++i)
        dest[i] = scopeBuffer[(size_t) ((start + i) & mask)];
}

void BassForgeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void BassForgeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* BassForgeAudioProcessor::createEditor()
{
    return new BassForgeAudioProcessorEditor (*this);
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassForgeAudioProcessor();
}
