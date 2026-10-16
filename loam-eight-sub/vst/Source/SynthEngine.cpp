#include "SynthEngine.h"

void SynthEngine::prepare (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    for (auto& v : voices)
        v = Voice {};

    z1 = z2 = 0.0;
    delayLine.reset();
    delayLine.setMaximumDelayInSamples ((int) (sampleRate * 1.0) + 4);
    delayLine.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });

    reverb.setSampleRate (sampleRate);
    reverb.reset();

    reverbScratchL.assign ((size_t) samplesPerBlock, 0.0f);
    reverbScratchR.assign ((size_t) samplesPerBlock, 0.0f);
}

SynthEngine::Voice* SynthEngine::findVoiceForNote (int note)
{
    for (auto& v : voices)
        if (v.active && v.note == note)
            return &v;
    return nullptr;
}

SynthEngine::Voice* SynthEngine::allocateVoice (int note)
{
    if (auto* existing = findVoiceForNote (note))
        return existing;

    for (auto& v : voices)
        if (! v.active)
            return &v;

    // No free voice: steal one already releasing, else the oldest slot.
    for (auto& v : voices)
        if (v.stage == Stage::Release)
            return &v;

    return &voices.front();
}

void SynthEngine::noteOn (int midiNote, float /*velocity*/)
{
    auto* v = allocateVoice (midiNote);
    v->active = true;
    v->note = midiNote;
    v->phase = 0.0;
    v->subPhase = 0.0;
    v->freq = 440.0 * std::pow (2.0, (midiNote - 69) / 12.0) * std::pow (2.0, tuneCents / 1200.0);
    v->stage = Stage::Attack;
    v->envLevel = 0.0f;
}

void SynthEngine::noteOff (int midiNote)
{
    if (auto* v = findVoiceForNote (midiNote))
    {
        v->releaseStartLevel = v->envLevel;
        v->stage = Stage::Release;
    }
}

void SynthEngine::allNotesOff()
{
    for (auto& v : voices)
        if (v.active)
        {
            v.releaseStartLevel = v.envLevel;
            v.stage = Stage::Release;
        }
}

void SynthEngine::updateFilterCoefficients()
{
    const double freq = juce::jlimit (20.0, sampleRate * 0.49, (double) cutoffHz);
    const double q = juce::jlimit (0.1, 20.0, (double) resonanceQ);
    const double omega = juce::MathConstants<double>::twoPi * freq / sampleRate;
    const double sn = std::sin (omega), cs = std::cos (omega);
    const double alpha = sn / (2.0 * q);

    const double a0 = 1.0 + alpha;
    b0 = ((1.0 - cs) / 2.0) / a0;
    b1 = (1.0 - cs) / a0;
    b2 = b0;
    a1 = (-2.0 * cs) / a0;
    a2 = (1.0 - alpha) / a0;
}

float SynthEngine::oscillatorSample (Wave w, double ph) const
{
    switch (w)
    {
        case Wave::Sine:     return (float) std::sin (juce::MathConstants<double>::twoPi * ph);
        case Wave::Saw:      return (float) (2.0 * ph - 1.0);
        case Wave::Square:   return ph < 0.5 ? 1.0f : -1.0f;
        case Wave::Triangle: return (float) (2.0 * std::abs (2.0 * ph - 1.0) - 1.0);
    }
    return 0.0f;
}

void SynthEngine::advanceEnvelope (Voice& v)
{
    switch (v.stage)
    {
        case Stage::Attack:
        {
            const double inc = 1.0 / (juce::jmax (1.0f, attackMs) / 1000.0 * sampleRate);
            v.envLevel += (float) inc;
            if (v.envLevel >= 1.0f) { v.envLevel = 1.0f; v.stage = Stage::Decay; }
            break;
        }
        case Stage::Decay:
        {
            const float target = sustainPct / 100.0f;
            const double dec = (1.0 - target) / (juce::jmax (1.0f, decayMs) / 1000.0 * sampleRate);
            v.envLevel -= (float) dec;
            if (v.envLevel <= target) { v.envLevel = target; v.stage = Stage::Sustain; }
            break;
        }
        case Stage::Sustain:
            v.envLevel = sustainPct / 100.0f;
            break;
        case Stage::Release:
        {
            const double dec = v.releaseStartLevel / (juce::jmax (1.0f, releaseMs) / 1000.0 * sampleRate);
            v.envLevel -= (float) dec;
            if (v.envLevel <= 0.0f) { v.envLevel = 0.0f; v.stage = Stage::Idle; v.active = false; }
            break;
        }
        case Stage::Idle:
            break;
    }
}

void SynthEngine::render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    updateFilterCoefficients();

    const float sizeNorm = juce::jlimit (0.0f, 1.0f, (reverbSizeSec - 0.3f) / (6.0f - 0.3f));
    const float toneNorm = juce::jlimit (0.0f, 1.0f, (reverbToneHz - 500.0f) / (12000.0f - 500.0f));
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = sizeNorm;
    rp.damping = 1.0f - toneNorm;
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    reverb.setParameters (rp);

    const double delaySamples = juce::jlimit (1.0, (double) delayLine.getMaximumDelayInSamples() - 1.0,
                                               (double) delayTimeMs / 1000.0 * sampleRate);
    const float fbGain = juce::jlimit (0.0f, 0.85f, delayFeedbackPct / 100.0f);
    const float delayWetGain = delayMixPct / 100.0f;
    const float reverbWetGain = reverbMixPct / 100.0f;
    const float masterGain = (volumePct / 100.0f) * 0.8f;
    const float subGain = subMixPct / 100.0f;

    auto* left = buffer.getWritePointer (0, startSample);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1, startSample) : left;

    jassert ((size_t) numSamples <= reverbScratchL.size());

    // Pass 1: voices -> shared filter -> dry + delay send straight into the
    // output buffer; the post-filter signal is also stashed for the reverb,
    // which (unlike the delay line) only offers block-based processing.
    for (int i = 0; i < numSamples; ++i)
    {
        float mixed = 0.0f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            const double dPhase = v.freq / sampleRate;
            const double dSubPhase = (v.freq * 0.5) / sampleRate;

            const float osc = oscillatorSample (wave, v.phase);
            const float sub = (float) std::sin (juce::MathConstants<double>::twoPi * v.subPhase);

            advanceEnvelope (v);
            mixed += (osc + subGain * sub) * v.envLevel;

            v.phase += dPhase;
            if (v.phase >= 1.0) v.phase -= 1.0;
            v.subPhase += dSubPhase;
            if (v.subPhase >= 1.0) v.subPhase -= 1.0;
        }

        // Shared lowpass filter (RBJ biquad, transposed direct form II).
        const double x = mixed;
        const double y = b0 * x + z1;
        z1 = b1 * x + z2 - a1 * y;
        z2 = b2 * x - a2 * y;
        const float filtered = (float) y;

        // Delay send (feedback line read-before-write).
        const float delayRead = delayLine.popSample (0, (float) delaySamples, true);
        delayLine.pushSample (0, filtered + delayRead * fbGain);
        const float delayWet = delayRead * delayWetGain;

        left[i] = (filtered + delayWet) * masterGain;
        right[i] = left[i];

        reverbScratchL[(size_t) i] = filtered;
        reverbScratchR[(size_t) i] = filtered;
    }

    // Pass 2: reverb runs block-at-a-time, then its wet signal is folded
    // into the output alongside the dry/delay signal already written above.
    reverb.processStereo (reverbScratchL.data(), reverbScratchR.data(), numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] += reverbScratchL[(size_t) i] * reverbWetGain * masterGain;
        right[i] += reverbScratchR[(size_t) i] * reverbWetGain * masterGain;
    }
}
