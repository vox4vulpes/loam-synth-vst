#include "SynthEngine.h"

void SynthEngine::prepare (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    for (auto& v : voices)
        v = Voice {};

    z1 = z2 = 0.0;
    satTone = {};
    distTone = {};
    reverbToneL = {};
    reverbToneR = {};
    delayLine.reset();
    delayLine.setMaximumDelayInSamples ((int) (sampleRate * 1.0) + 4);
    delayLine.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 1 });

    reverb.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
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

    // No free voice: steal one already releasing, then one only the sustain
    // pedal is holding, else the oldest slot.
    for (auto& v : voices)
        if (v.stage == Stage::Release)
            return &v;

    for (auto& v : voices)
        if (! v.keyDown)
            return &v;

    return &voices.front();
}

void SynthEngine::noteOn (int midiNote, float /*velocity*/)
{
    if (isPortaNote (midiNote))
    {
        portaNoteOn (midiNote);
        return;
    }

    auto* v = allocateVoice (midiNote);
    v->active = true;
    v->keyDown = true;
    v->porta = false;
    v->note = midiNote;
    v->phase = 0.0;
    v->subPhase = 0.0;
    v->pitch = v->targetPitch = midiNote + tuneCents / 100.0;
    v->pitchStep = 0.0;
    v->freq = pitchToFreq (v->pitch + lastPitchOffset);
    v->stage = Stage::Attack;
    v->envLevel = 0.0f;
}

//==============================================================================
void SynthEngine::glideTo (Voice& v, double target)
{
    v.targetPitch = target;
    const auto samples = (double) portaTimeMs / 1000.0 * sampleRate;
    if (samples < 1.0)
    {
        v.pitch = target;
        v.pitchStep = 0.0;
    }
    else
    {
        v.pitchStep = (target - v.pitch) / samples;
    }
    v.freq = pitchToFreq (v.pitch + lastPitchOffset);
}

void SynthEngine::portaNoteOn (int note)
{
    // Push onto the held stack (dropping the oldest if it's full).
    if (portaHeldCount == portaStackSize)
    {
        std::copy (portaHeld.begin() + 1, portaHeld.end(), portaHeld.begin());
        --portaHeldCount;
    }
    portaHeld[(size_t) portaHeldCount++] = note;

    const auto target = note + tuneCents / 100.0;
    Voice* v = (portaVoice >= 0 && voices[(size_t) portaVoice].active && voices[(size_t) portaVoice].porta)
                   ? &voices[(size_t) portaVoice] : nullptr;

    if (v != nullptr && v->stage != Stage::Release)
    {
        // Legato: the voice is still sounding, so just glide to the new key.
        glideTo (*v, target);
    }
    else
    {
        // Start from where the last glide note was, so even detached notes slide.
        const auto from = v != nullptr ? v->pitch : (lastPortaPitch >= 0.0 ? lastPortaPitch : target);
        if (v == nullptr)
        {
            v = allocateVoice (note);
            v->phase = 0.0;
            v->subPhase = 0.0;
            v->envLevel = 0.0f;
        }
        v->active = true;
        v->porta = true;
        v->stage = Stage::Attack;
        v->pitch = from;
        glideTo (*v, target);
        portaVoice = (int) (v - voices.data());
    }

    v->note = note;
    v->keyDown = true;
    lastPortaPitch = target;
}

bool SynthEngine::portaNoteOff (int note)
{
    int index = -1;
    for (int i = 0; i < portaHeldCount; ++i)
        if (portaHeld[(size_t) i] == note)
            index = i;
    if (index < 0)
        return false;

    std::copy (portaHeld.begin() + index + 1, portaHeld.begin() + portaHeldCount, portaHeld.begin() + index);
    --portaHeldCount;

    if (portaVoice < 0)
        return true;

    auto& v = voices[(size_t) portaVoice];
    if (! v.active || ! v.porta || v.note != note)
        return true;   // an older key in the stack, not the one sounding

    if (portaHeldCount > 0)
    {
        // Glide back to the most recent key still held.
        const auto back = portaHeld[(size_t) portaHeldCount - 1];
        v.note = back;
        lastPortaPitch = back + tuneCents / 100.0;
        glideTo (v, lastPortaPitch);
    }
    else if (sustainDown)
    {
        v.keyDown = false;
    }
    else
    {
        startRelease (v);
    }
    return true;
}

int SynthEngine::getNumActiveVoices() const
{
    int n = 0;
    for (const auto& v : voices)
        if (v.active)
            ++n;
    return n;
}

// RBJ cookbook lowpass/highpass with Q = 1/sqrt(2) (Butterworth). The web's
// BiquadFilterNode uses the same formula with Q given in dB, so it sets -3.01 dB.
void SynthEngine::Biquad::setTone (double hz, bool highPass, double sr)
{
    const auto w0 = juce::MathConstants<double>::twoPi * juce::jlimit (20.0, 0.45 * sr, hz) / sr;
    const auto cosW = std::cos (w0);
    const auto a = std::sin (w0) / (2.0 * 0.7071067811865476);   // alpha for Q = 1/sqrt(2)
    const auto a0 = 1.0 + a;
    if (highPass)
    {
        b0 = (1.0 + cosW) / 2.0 / a0;
        b1 = -(1.0 + cosW) / a0;
    }
    else
    {
        b0 = (1.0 - cosW) / 2.0 / a0;
        b1 = (1.0 - cosW) / a0;
    }
    b2 = b0;
    a1 = -2.0 * cosW / a0;
    a2 = (1.0 - a) / a0;
}

double SynthEngine::Biquad::process (double x)
{
    const auto y = b0 * x + z1;
    z1 = b1 * x + z2 - a1 * y;
    z2 = b2 * x - a2 * y;
    return y;
}

float SynthEngine::processDrive (float x, const Drive& d, bool hardClip, Biquad& tone) const
{
    if (! d.on)
        return x;

    double wet;
    if (hardClip)
    {
        // Distortion: up to 40x gain into an asymmetric clip (a touch of even harmonics).
        const auto g = 1.0 + d.drivePct / 100.0 * 39.0;
        const auto y = juce::jlimit (-0.6, 0.8, g * x);
        wet = y * std::pow (g, -0.35);
    }
    else
    {
        // Saturation: up to 10x gain into tanh, scaled back so it stays near the dry level.
        const auto g = 1.0 + d.drivePct / 100.0 * 9.0;
        wet = std::tanh (g * x) / std::sqrt (g);
    }

    const auto filtered = tone.process (wet);
    const auto mix = d.mixPct / 100.0;
    return (float) (x * (1.0 - mix) + filtered * mix);
}

void SynthEngine::startRelease (Voice& v)
{
    v.keyDown = false;
    if (v.stage == Stage::Release || v.stage == Stage::Idle)
        return;
    v.releaseStartLevel = v.envLevel;
    v.stage = Stage::Release;
}

void SynthEngine::noteOff (int midiNote)
{
    if (portaNoteOff (midiNote))
        return;

    if (auto* v = findVoiceForNote (midiNote))
    {
        if (sustainDown)
            v->keyDown = false;   // the pedal keeps it sounding until it lifts
        else
            startRelease (*v);
    }
}

void SynthEngine::setSustain (bool down)
{
    if (down == sustainDown)
        return;

    sustainDown = down;
    if (! down)
        for (auto& v : voices)
            if (v.active && ! v.keyDown)
                startRelease (v);
}

void SynthEngine::allNotesOff()
{
    portaHeldCount = 0;
    for (auto& v : voices)
        if (v.active)
            startRelease (v);
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

    // Reverb TONE is a filter on the reverb's output (as on the web), so the
    // reverb itself keeps a fixed, moderate damping.
    const float sizeNorm = juce::jlimit (0.0f, 1.0f, (reverbSizeSec - 0.3f) / (6.0f - 0.3f));
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = sizeNorm;
    rp.damping = 0.35f;
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
    satTone.setTone (saturation.toneHz, saturation.toneHighPass, sampleRate);
    distTone.setTone (distortion.toneHz, distortion.toneHighPass, sampleRate);

    // ~5 ms smoothing so wheel steps don't click; vibrato LFO at 5.5 Hz.
    const double bendCoef = 1.0 - std::exp (-1.0 / (0.005 * sampleRate));
    const double lfoStep = 5.5 / sampleRate;

    for (int i = 0; i < numSamples; ++i)
    {
        float mixed = 0.0f;

        smoothedBend += ((double) bendSemis - smoothedBend) * bendCoef;
        lfoPhase += lfoStep;
        if (lfoPhase >= 1.0) lfoPhase -= 1.0;
        const double pitchOffset = smoothedBend
                                 + (double) vibratoSemis * std::sin (juce::MathConstants<double>::twoPi * lfoPhase);
        const bool offsetMoving = std::abs (pitchOffset - lastPitchOffset) > 1.0e-6;
        lastPitchOffset = pitchOffset;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            const bool gliding = v.pitchStep != 0.0;
            if (gliding)
            {
                v.pitch += v.pitchStep;
                if ((v.pitchStep > 0.0 && v.pitch >= v.targetPitch) || (v.pitchStep < 0.0 && v.pitch <= v.targetPitch))
                {
                    v.pitch = v.targetPitch;
                    v.pitchStep = 0.0;
                }
            }
            if (gliding || offsetMoving)
                v.freq = pitchToFreq (v.pitch + pitchOffset);

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
        float filtered = (float) y;
        filtered = processDrive (filtered, saturation, false, satTone);
        filtered = processDrive (filtered, distortion, true, distTone);

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
    float* reverbChannels[2] = { reverbScratchL.data(), reverbScratchR.data() };
    juce::dsp::AudioBlock<float> reverbBlock (reverbChannels, 2, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> reverbContext (reverbBlock);
    reverb.process (reverbContext);
    reverbToneL.setTone (reverbToneHz, reverbToneHighPass, sampleRate);
    reverbToneR.setTone (reverbToneHz, reverbToneHighPass, sampleRate);
    for (int i = 0; i < numSamples; ++i)
    {
        left[i] += (float) reverbToneL.process (reverbScratchL[(size_t) i]) * reverbWetGain * masterGain;
        right[i] += (float) reverbToneR.process (reverbScratchR[(size_t) i]) * reverbWetGain * masterGain;
    }
}
