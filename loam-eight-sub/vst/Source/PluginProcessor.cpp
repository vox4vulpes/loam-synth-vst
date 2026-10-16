#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr auto waveId = "wave", tuneId = "tune", subId = "sub", cutoffId = "cutoff",
                    resonanceId = "resonance", attackId = "attack", decayId = "decay",
                    sustainId = "sustain", releaseId = "release", volumeId = "volume",
                    delayTimeId = "delayTime", delayFeedbackId = "delayFeedback", delayMixId = "delayMix",
                    reverbMixId = "reverbMix", reverbSizeId = "reverbSize", reverbToneId = "reverbTone";
}

LoamEightSubProcessor::LoamEightSubProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout LoamEightSubProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID (waveId, 1), "Wave",
        juce::StringArray { "Sine", "Saw", "Square", "Triangle" }, 1));

    params.push_back (std::make_unique<Param> (juce::ParameterID (tuneId, 1), "Tune", Range (-100.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (subId, 1), "Sub", Range (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (cutoffId, 1), "Cutoff", Range (80.0f, 12000.0f), 2200.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (resonanceId, 1), "Resonance", Range (0.1f, 20.0f), 1.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (attackId, 1), "Attack", Range (1.0f, 2000.0f), 10.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (decayId, 1), "Decay", Range (1.0f, 2000.0f), 150.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (sustainId, 1), "Sustain", Range (0.0f, 100.0f), 70.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (releaseId, 1), "Release", Range (1.0f, 3000.0f), 300.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (volumeId, 1), "Volume", Range (0.0f, 100.0f), 70.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (delayTimeId, 1), "Delay Time", Range (20.0f, 800.0f), 220.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (delayFeedbackId, 1), "Delay Feedback", Range (0.0f, 85.0f), 30.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (delayMixId, 1), "Delay Mix", Range (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (reverbMixId, 1), "Reverb Mix", Range (0.0f, 100.0f), 0.0f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (reverbSizeId, 1), "Reverb Size", Range (0.3f, 6.0f), 2.2f));
    params.push_back (std::make_unique<Param> (juce::ParameterID (reverbToneId, 1), "Reverb Tone", Range (500.0f, 12000.0f), 6000.0f));

    return { params.begin(), params.end() };
}

void LoamEightSubProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
}

void LoamEightSubProcessor::pullParameters()
{
    engine.wave = (SynthEngine::Wave) (int) apvts.getRawParameterValue (waveId)->load();
    engine.tuneCents = apvts.getRawParameterValue (tuneId)->load();
    engine.subMixPct = apvts.getRawParameterValue (subId)->load();
    engine.cutoffHz = apvts.getRawParameterValue (cutoffId)->load();
    engine.resonanceQ = apvts.getRawParameterValue (resonanceId)->load();
    engine.attackMs = apvts.getRawParameterValue (attackId)->load();
    engine.decayMs = apvts.getRawParameterValue (decayId)->load();
    engine.sustainPct = apvts.getRawParameterValue (sustainId)->load();
    engine.releaseMs = apvts.getRawParameterValue (releaseId)->load();
    engine.volumePct = apvts.getRawParameterValue (volumeId)->load();
    engine.delayTimeMs = apvts.getRawParameterValue (delayTimeId)->load();
    engine.delayFeedbackPct = apvts.getRawParameterValue (delayFeedbackId)->load();
    engine.delayMixPct = apvts.getRawParameterValue (delayMixId)->load();
    engine.reverbMixPct = apvts.getRawParameterValue (reverbMixId)->load();
    engine.reverbSizeSec = apvts.getRawParameterValue (reverbSizeId)->load();
    engine.reverbToneHz = apvts.getRawParameterValue (reverbToneId)->load();
}

void LoamEightSubProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    pullParameters();

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
            engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff())
            engine.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            engine.allNotesOff();
    }

    engine.render (buffer, 0, buffer.getNumSamples());
}

bool LoamEightSubProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

juce::AudioProcessorEditor* LoamEightSubProcessor::createEditor()
{
    return new LoamEightSubEditor (*this);
}

void LoamEightSubProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LoamEightSubProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoamEightSubProcessor();
}
